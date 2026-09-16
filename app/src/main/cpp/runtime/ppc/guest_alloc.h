// guest_alloc.h — alocadores da memória guest (endereços VA do guest)
//
// Ambos os alocadores gerenciam JANELAS do espaço de endereços guest
// (uint64_t) e guardam TODA a metadada no host (std::map) — nada é escrito
// na memória do guest além dos bytes que o próprio guest pedir. Thread-safe
// (mutex interno): chamados por múltiplas threads guest simultâneas.
//
//  - GuestHeap: heap do guest para ExAllocatePool/ExFreePool (granularidade
//    16 bytes, first-fit com coalescência — sem overhead dentro do guest).
//  - GuestVirtWindow: janela de "memória virtual/física" para
//    NtAllocateVirtualMemory, MmAllocatePhysicalMemoryEx e kernel stacks
//    (granularidade de página 4 KB, aceita alocação em endereço específico).
//
// Nada simulado: falha de espaço real retorna 0 (NULL) / erro — o comportamento
// que o kernel real teria ao esgotar a região.
#pragma once

#include <cstdint>
#include <map>
#include <mutex>
#include <optional>
#include <vector>

namespace fh2::ppc {

class GuestHeap {
public:
    void init(uint64_t base, uint64_t bytes);   // idempotente

    /** Volta o alocador ao estado pós-init (relaunch do título: o boot do
     *  guest recria TUDO, então nenhuma alocação anterior pode sobreviver). */
    void reset();
    /** Aloca `size` bytes alinhados a `align` (padrão 16). 0 = falhou. */
    uint64_t alloc(uint64_t size, uint64_t align = 16);
    /** Libera um bloco devolvido por alloc() (no-op seguro se inválido). */
    void free(uint64_t addr);

    uint64_t base() const { return base_; }
    uint64_t size() const { return size_; }
    uint64_t used() const;

private:
    mutable std::mutex m_;
    uint64_t base_ = 0, size_ = 0;
    uint64_t nextFree_ = 0;              // cursor de bump
    std::map<uint64_t, uint64_t> freeRanges_;  // addr -> size (livres pós-bump)
    std::map<uint64_t, uint64_t> blocks_;      // addr -> size (em uso)
};

class GuestVirtWindow {
public:
    void init(uint64_t base, uint64_t bytes, uint64_t page = 0x1000);

    /** Volta o alocador ao estado pós-init (relaunch do título). */
    void reset();
    /** Aloca `size` bytes alinhados à página (ou `align` se maior).
     *  0 = falhou. */
    uint64_t alloc(uint64_t size, uint64_t align = 0);
    /** Aloca em endereço específico (NtAllocateVirtualMemory com base
     *  preferida). 0 = falhou (faixa ocupada/fora da janela). */
    uint64_t allocAt(uint64_t addr, uint64_t size);
    void free(uint64_t addr);

    /** Tamanho do bloco que contém addr (0 se nenhum). */
    uint64_t blockSize(uint64_t addr) const;

    /** Região de endereçamento contendo `addr` DENTRO da janela:
     *  base/size do bloco comprometido (committed=true) ou do vão livre
     *  (committed=false). false = addr fora da janela. */
    bool regionAt(uint64_t addr, uint64_t& base, uint64_t& size,
                  bool& committed) const;

    /** true se [addr, addr+size) está inteiramente dentro de um bloco já
     *  alocado (usado p/ COMMIT dentro de RESERVE — kernel real permite). */
    bool containsRange(uint64_t addr, uint64_t size) const;

    uint64_t base() const { return base_; }
    uint64_t size() const { return size_; }
    uint64_t used() const;

private:
    mutable std::mutex m_;
    uint64_t base_ = 0, size_ = 0, page_ = 0x1000;
    uint64_t nextFree_ = 0;
    std::map<uint64_t, uint64_t> freeRanges_;
    std::map<uint64_t, uint64_t> blocks_;      // addr -> size (páginas)
};

} // namespace fh2::ppc
