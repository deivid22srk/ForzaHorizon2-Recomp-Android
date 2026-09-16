// guest_alloc.cpp — implementação dos alocadores de memória guest
//
// Estratégia bump + free-list host-side: alocar avança um cursor; free()
// devolve a faixa ao mapa de livres com coalescência de vizinhos. Toda a
// contabilidade vive no HOST (std::map) — a memória guest não guarda
// cabeçalhos, exatamente como o kernel real gerencia atrás do guest.
#include "guest_alloc.h"

#include <algorithm>

namespace fh2::ppc {

// ---------------------------------------------------------------- GuestHeap

void GuestHeap::init(uint64_t base, uint64_t bytes) {
    std::lock_guard<std::mutex> lk(m_);
    if (base_ != 0) return; // idempotente
    base_ = base;
    size_ = bytes;
    nextFree_ = base;
    freeRanges_.clear();
    blocks_.clear();
}

uint64_t GuestHeap::used() const {
    std::lock_guard<std::mutex> lk(m_);
    uint64_t used = nextFree_ - base_;
    for (auto& [a, s] : blocks_) used += s;
    return used;
}

uint64_t GuestHeap::alloc(uint64_t size, uint64_t align) {
    if (size == 0) size = 1;
    if (align < 16) align = 16;
    align = std::max<uint64_t>(align, 16);

    std::lock_guard<std::mutex> lk(m_);
    if (base_ == 0) return 0;

    // 1) tenta first-fit nas faixas liberadas
    for (auto it = freeRanges_.begin(); it != freeRanges_.end(); ++it) {
        const uint64_t rangeStart = it->first;
        const uint64_t rangeSize = it->second;
        const uint64_t aligned =
            (rangeStart + (align - 1)) & ~(align - 1);
        const uint64_t pad = aligned - rangeStart;
        if (pad + size > rangeSize) continue;

        const uint64_t rest = rangeSize - pad - size;
        freeRanges_.erase(it);
        if (rest > 0) freeRanges_[aligned + size] = rest;
        blocks_[aligned] = size;
        return aligned;
    }

    // 2) bump no cursor
    const uint64_t aligned = (nextFree_ + (align - 1)) & ~(align - 1);
    if (aligned < nextFree_ || aligned + size > base_ + size_) return 0;
    nextFree_ = aligned + size;
    blocks_[aligned] = size;
    return aligned;
}

void GuestHeap::free(uint64_t addr) {
    if (addr == 0) return;
    std::lock_guard<std::mutex> lk(m_);
    auto it = blocks_.find(addr);
    if (it == blocks_.end()) return; // free inválido: ignora (kernel logaria)
    const uint64_t size = it->second;
    blocks_.erase(it);

    // coalescência com vizinhos imediatos
    auto merge = [&](uint64_t& start, uint64_t& len) {
        auto next = freeRanges_.find(start + len);
        if (next != freeRanges_.end()) {
            len += next->second;
            freeRanges_.erase(next);
        }
        // std::prev(begin()) é UB (árvore vazia/primeiro elemento) —
        // verificar begin() ANTES de retroceder o iterador.
        auto prev = freeRanges_.lower_bound(start);
        if (prev != freeRanges_.begin()) {
            --prev;
            if (prev->first + prev->second == start) {
                start = prev->first;
                len += prev->second;
                freeRanges_.erase(prev);
            }
        }
    };
    uint64_t start = addr, len = size;
    merge(start, len);
    freeRanges_[start] = len;
}

// --------------------------------------------------------- GuestVirtWindow

void GuestVirtWindow::init(uint64_t base, uint64_t bytes, uint64_t page) {
    std::lock_guard<std::mutex> lk(m_);
    if (base_ != 0) return;
    base_ = base;
    size_ = bytes;
    page_ = page ? page : 0x1000;
    nextFree_ = base;
    freeRanges_.clear();
    blocks_.clear();
}

uint64_t GuestVirtWindow::used() const {
    std::lock_guard<std::mutex> lk(m_);
    uint64_t used = nextFree_ - base_;
    for (auto& [a, s] : blocks_) used += s;
    return used;
}

uint64_t GuestVirtWindow::alloc(uint64_t size, uint64_t align) {
    if (size == 0) size = page_;
    size = (size + page_ - 1) & ~(page_ - 1);
    if (align < page_) align = page_;
    align = (align + page_ - 1) & ~(page_ - 1);

    std::lock_guard<std::mutex> lk(m_);
    if (base_ == 0) return 0;

    for (auto it = freeRanges_.begin(); it != freeRanges_.end(); ++it) {
        const uint64_t aligned = (it->first + align - 1) & ~(align - 1);
        const uint64_t pad = aligned - it->first;
        if (pad + size > it->second) continue;
        const uint64_t addr = aligned;
        const uint64_t rest = it->second - pad - size;
        const uint64_t rangeStart = it->first;
        freeRanges_.erase(it);
        if (pad > 0) freeRanges_[rangeStart] = pad;
        if (rest > 0) freeRanges_[addr + size] = rest;
        blocks_[addr] = size;
        return addr;
    }
    const uint64_t aligned = (nextFree_ + align - 1) & ~(align - 1);
    if (aligned < nextFree_ || aligned + size > base_ + size_) return 0;
    if (aligned > nextFree_) {
        // buraco de alinhamento entre o cursor e o bloco volta como livre
        freeRanges_[nextFree_] = aligned - nextFree_;
    }
    nextFree_ = aligned + size;
    blocks_[aligned] = size;
    return aligned;
}

uint64_t GuestVirtWindow::allocAt(uint64_t addr, uint64_t size) {
    if (size == 0) size = page_;
    size = (size + page_ - 1) & ~(page_ - 1);
    addr = addr & ~(page_ - 1);

    std::lock_guard<std::mutex> lk(m_);
    if (base_ == 0) return 0;
    if (addr < base_ || addr + size > base_ + size_) return 0;

    // sobrepõe com algum bloco em uso?
    if (!blocks_.empty()) {
        auto first = blocks_.lower_bound(addr);
        if (first != blocks_.end() && first->first < addr + size) return 0;
        if (first != blocks_.begin()) {
            auto prev = std::prev(first);
            if (prev->first + prev->second > addr) return 0;
        }
    }
    // recorta das faixas livres que intersectam [addr, addr+size)
    std::vector<std::pair<uint64_t, uint64_t>> keep;
    auto it = freeRanges_.begin();
    while (it != freeRanges_.end()) {
        const uint64_t rs = it->first, re = it->first + it->second;
        if (re <= addr || rs >= addr + size) { ++it; continue; }
        if (rs < addr) keep.emplace_back(rs, addr - rs);          // parte antes
        if (re > addr + size) keep.emplace_back(addr + size, re - (addr + size));
        it = freeRanges_.erase(it);
    }
    for (auto& [s, l] : keep) freeRanges_[s] = l;
    blocks_[addr] = size;
    return addr;
}

void GuestVirtWindow::free(uint64_t addr) {
    if (addr == 0) return;
    std::lock_guard<std::mutex> lk(m_);
    auto it = blocks_.find(addr);
    if (it == blocks_.end()) return;
    const uint64_t size = it->second;
    blocks_.erase(it);
    // coalescência com vizinhos imediatos
    uint64_t start = addr, len = size;
    {
        auto next = freeRanges_.find(start + len);
        if (next != freeRanges_.end()) {
            len += next->second;
            freeRanges_.erase(next);
        }
        // std::prev(begin()) é UB (árvore vazia/primeiro elemento)
        auto prev = freeRanges_.lower_bound(start);
        if (prev != freeRanges_.begin()) {
            --prev;
            if (prev->first + prev->second == start) {
                start = prev->first;
                len += prev->second;
                freeRanges_.erase(prev);
            }
        }
    }
    freeRanges_[start] = len;
}

uint64_t GuestVirtWindow::blockSize(uint64_t addr) const {
    std::lock_guard<std::mutex> lk(m_);
    auto it = blocks_.find(addr & ~(page_ - 1));
    return it == blocks_.end() ? 0 : it->second;
}

bool GuestVirtWindow::regionAt(uint64_t addr, uint64_t& base, uint64_t& size,
                               bool& committed) const {
    std::lock_guard<std::mutex> lk(m_);
    if (base_ == 0 || addr < base_ || addr >= base_ + size_) return false;

    // bloco comprometido contendo addr?
    auto next = blocks_.upper_bound(addr);
    if (next != blocks_.begin()) {
        auto prev = std::prev(next);
        const uint64_t b = prev->first, e = b + prev->second;
        if (addr < e) {
            base = b;
            size = e - b;
            committed = true;
            return true;
        }
    }
    // vão livre: do fim do bloco anterior (ou início da janela) até o
    // início do próximo bloco (ou fim da janela)
    uint64_t lo = base_, hi = base_ + size_;
    if (next != blocks_.begin()) {
        auto prev = std::prev(next);
        lo = prev->first + prev->second;
    }
    if (next != blocks_.end()) hi = next->first;
    if (lo < base_) lo = base_;
    if (hi > base_ + size_) hi = base_ + size_;
    if (addr < lo || addr >= hi) return false; // não deveria ocorrer
    base = lo;
    size = hi - lo;
    committed = false;
    return true;
}

bool GuestVirtWindow::containsRange(uint64_t addr, uint64_t size) const {
    std::lock_guard<std::mutex> lk(m_);
    if (base_ == 0 || size == 0) return false;
    auto it = blocks_.upper_bound(addr);
    if (it == blocks_.begin()) return false;
    --it;
    const uint64_t b = it->first, e = b + it->second;
    return addr >= b && addr + size <= e;
}

} // namespace fh2::ppc
