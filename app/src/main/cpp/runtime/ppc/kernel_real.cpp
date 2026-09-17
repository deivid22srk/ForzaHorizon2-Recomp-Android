// kernel_real.cpp — implementações REAIS dos imports do xboxkrnl (issue #16)
//
// Semântica implementada de verdade:
//   • heap/janela virtual: ExAllocatePool*/ExFreePool, NtAllocate/FreeVirtualMemory,
//     MmAllocatePhysicalMemoryEx/MmFreePhysicalMemory/MmGetPhysicalAddress,
//     MmCreateKernelStack/MmDeleteKernelStack (guest_alloc)
//   • tempo real: KeQuerySystemTime (CLOCK_REALTIME), KeQueryPerformanceFrequency
//   • espera real: KeDelayExecutionThread (sleep com unwind no stop)
//   • sincronização real: events/semaphores/mutants (mutex+condvar do host),
//     critical sections Rtl*
//   • threads guest reais: ExCreateThread/KeResumeThread/ExTerminateThread
//   • TLS do kernel: KeTls*
//   • printf/DbgPrint reais lendo memória guest (logs de boot do jogo)
//   • input real: XamInputGetState reflete HUD touch + gamepads físicos
//   • conversões Rtl de tempo/strings (ANSI↔Unicode) reais
//
// Convenção de args: r3..r10; big-endian; retornos em r3 (u32) / r3:r4 (u64).
#include "kernel_real.h"
#include "runtime/ppc/xex_loader.h"

#if FH2_HAS_RECOMP

#include <android/log.h>
#include "ppc_context.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <map>
#include <mutex>
#include <thread>
#include <unistd.h>

#include "runtime/input/input_state.h"
#include "runtime/gfx/graphics_backend.h"
#include "runtime/gpu/cmd_processor.h"
#include "runtime/ppc/guest_entry.h"
#include "runtime/ppc/kernel_state.h"

#define RLOG(...) __android_log_print(ANDROID_LOG_INFO, "FH2/KRN", __VA_ARGS__)

// ---- diagnóstico de código não recompilado (chamada indireta guardada) ----
// Endereço alvo da última chamada indireta fora da faixa de código gerado
// (ex.: export de módulo XEX secundário carregado em runtime). O thunk loga
// (throttled) e retorna r3 = 0 — comportamento DEFINIDO e visível; nunca um
// SIGSEGV selvagem na tabela mágica.
extern "C" uint32_t fh2_guest_unmappedTarget = 0;
extern "C" void fh2_guest_unmapped_code(PPCContext& ctx, uint8_t* base) {
    (void)base;
    static std::atomic<uint32_t> last{0};
    static std::atomic<int> count{0};
    const uint32_t ea = fh2_guest_unmappedTarget;
    ctx.r3.u64 = 0;
    if (last.load() != ea) {
        last.store(ea);
        // stubs sintéticos das tabelas completas: o nome REAL da função
        // resolvida via XexGetProcedureAddress (r3=0 = "recurso ausente")
        if (const char* name = fh2::kern::syntheticStubName(ea)) {
            RLOG("CHAMADA a export sintética %s (stub 0x%08X) → r3=0 "
                 "(semântica: recurso/serviço ausente)", name, ea);
        } else {
            RLOG("CHAMADA p/ código não recompilado: 0x%08X (módulo XEX "
                 "secundário fora do pipeline — ver tools/run_recomp.sh)", ea);
        }
    }
    count.fetch_add(1);
}

namespace fh2::kern {

namespace {

// ----- acesso à memória guest (big-endian) -----
// CRÍTICO: endereços guest chegam em registradores 64-bit com EXTENSÃO DE
// SINAL (o PPC64 do Xenon mantém 0xFFFFFFFF8xxxxxxx em registradores —
// comprovado no tombstone: base + 0xFFFFFFFF8342B6C0 → SEGV em 0x6e1fc2d6c0,
// real_ExCreateThread+156). O endereçamento REAL do guest é 32-bit: mascarar
// AQUI no ponto único de acesso — nenhum callee precisa lembrar disso.
inline uint32_t g32(uint8_t* b, uint64_t a) {
    return __builtin_bswap32(*(uint32_t*)(b + (uint32_t)a));
}
inline void w32(uint8_t* b, uint64_t a, uint32_t v) {
    *(uint32_t*)(b + (uint32_t)a) = __builtin_bswap32(v);
}
inline uint64_t g64(uint8_t* b, uint64_t a) {
    return __builtin_bswap64(*(uint64_t*)(b + (uint32_t)a));
}
inline void w64(uint8_t* b, uint64_t a, uint64_t v) {
    *(uint64_t*)(b + (uint32_t)a) = __builtin_bswap64(v);
}

inline bool validGuest(uint64_t a, uint64_t n = 4) {
    // O guest usa EAs de 32 bits; registradores 64-bit podem conter extensão
    // de sinal (ex.: 0xFFFFFFFF820CF1CC) que o endereçamento real ignora —
    // mascara para 32 bits antes de validar (mesma semântica do PPC_EA do
    // código gerado).
    const uint64_t a32 = (uint32_t)a;
    return guestMem() && a32 != 0 && a32 + n <= guestMemBytes();
}

// ----- varargs da ABI Xenon: inteiros r4..r10 (r3 = fmt/fixo), depois
// stack [r1+0x48], slot de 8 bytes; doubles em paralelo em f1..f13 -----
struct ArgSource {
    PPCContext& ctx;
    uint8_t* base;
    int intBase = 4;     // primeiro registrador de varargs
    int fpBase = 1;      // f1..f13
    static constexpr int kMaxRegInts = 7; // r4..r10

    uint64_t nextInt() {
        // índice lógico via contagem separada
        const int i = intCount++;
        if (i < kMaxRegInts) {
            switch (intBase + i) {
            case 4: return ctx.r4.u64;
            case 5: return ctx.r5.u64;
            case 6: return ctx.r6.u64;
            case 7: return ctx.r7.u64;
            case 8: return ctx.r8.u64;
            case 9: return ctx.r9.u64;
            default: return ctx.r10.u64;
            }
        }
        const int extra = i - kMaxRegInts;
        const uint64_t slot = ctx.r1.u64 + 0x48 + 8ull * extra;
        return validGuest(slot, 8) ? g64(base, slot) : 0;
    }
    double nextDouble() {
        const int i = fpCount++;
        switch (fpBase + i) {
        case 1: return ctx.f1.f64;
        case 2: return ctx.f2.f64;
        case 3: return ctx.f3.f64;
        case 4: return ctx.f4.f64;
        case 5: return ctx.f5.f64;
        case 6: return ctx.f6.f64;
        case 7: return ctx.f7.f64;
        case 8: return ctx.f8.f64;
        case 9: return ctx.f9.f64;
        case 10: return ctx.f10.f64;
        case 11: return ctx.f11.f64;
        case 12: return ctx.f12.f64;
        default: return ctx.f13.f64;
        }
    }
    int intCount = 0;
    int fpCount = 0;
};

GuestArgs makeArgs(ArgSource& src) {
    GuestArgs a;
    a.nextInt = [&src](int) -> uint64_t { return src.nextInt(); };
    a.nextDouble = [&src](int) -> double { return src.nextDouble(); };
    return a;
}

// ----- waitables -----
inline Waitable* ev(uint8_t* base, uint64_t obj, bool create) {
    return waitable(obj, create, WaitKind::Event);
}

// timeout: PLARGE_INTEGER (unidades de 100 ns; negativo = RELATIVO,
// positivo = ABSOLUTO desde 1601-01-01). Ponteiro NULL/inválido = espera
// INFINITA — sentinela kNoTimeout (INT64_MIN, definida em kernel_state.h).
// NUNCA -1 como "infinito": no NT, intervalo relativo -1 é um wait real de
// 100ns — o antigo sentinel -1 fazia waitForMultiple expirar em 1ns → spin
// de 1.6M iterações/s do worker do FH2 (log 16_09:
// NtWaitForSingleObjectEx(A0000000) → 0x102 instantâneo).
inline int64_t readTimeout(uint8_t* base, uint64_t addr) {
    if (addr == 0 || !validGuest(addr, 8)) return kNoTimeout; // infinito
    return (int64_t)g64(base, addr);
}

// log throttling p/ caminhos quentes: primeiros N + a cada 512º
struct Throttle {
    std::atomic<uint64_t> n{0};
    bool shouldLog(uint64_t first, uint64_t every) {
        const uint64_t i = n.fetch_add(1) + 1;
        return i <= first || (i % every) == 0;
    }
};

// STATUS constants reais do NT (usadas pelos helpers de espera abaixo)
constexpr uint32_t STATUS_SUCCESS = 0x00000000;
constexpr uint32_t STATUS_INVALID_PARAMETER = 0xC000000D;
constexpr uint32_t STATUS_INVALID_HANDLE = 0xC0000008;
constexpr uint32_t STATUS_ACCESS_DENIED = 0xC0000022;
constexpr uint32_t STATUS_OBJECT_NAME_NOT_FOUND = 0xC0000034;
constexpr uint32_t STATUS_NO_MEMORY = 0xC0000017;
constexpr uint32_t STATUS_UNSUCCESSFUL = 0xC0000001;
constexpr uint32_t STATUS_ACCESS_VIOLATION = 0xC0000005;

inline uint32_t waitOne(uint8_t* base, Waitable* w, uint64_t timeoutAddr,
                        bool alertable) {
    Waitable* objs[1] = {w};
    if (!w) return STATUS_INVALID_HANDLE; // semântica NT real (não timeout)
    return waitForMultiple(objs, 1, true, readTimeout(base, timeoutAddr),
                           alertable);
}

thread_local uint64_t t_kthreadBlock = 0; // bloco real p/ o KTHREAD opaco

// blocos-opaco por handle de ObReferenceObjectByHandle (estáveis)
std::map<uint32_t, uint64_t> g_obBlocks;
std::mutex g_obM;

} // namespace

// ------------------------------------------------------------- memória

void real_ExAllocatePool(PPCContext& ctx, uint8_t* base) {
    static Throttle t;
    const bool log = t.shouldLog(16, 512);
    const uint64_t size = ctx.r3.u64;
    const uint64_t p = heap().alloc(size ? size : 1, 16);
    ctx.r3.u64 = p;
    if (log || p == 0) {
        RLOG("ExAllocatePool(%llu) = 0x%08X%s", (unsigned long long)size,
             (unsigned)ctx.r3.u32, p == 0 ? " — HEAP ESGOTADO" : "");
    }
}

void real_ExAllocatePoolTypeWithTag(PPCContext& ctx, uint8_t* base) {
    static Throttle t;
    const bool log = t.shouldLog(16, 512);
    // (Type, Size, Tag) — Size em r4
    const uint64_t size = ctx.r4.u64;
    const uint64_t p = heap().alloc(size ? size : 1, 16);
    ctx.r3.u64 = p;
    if (log || p == 0) {
        RLOG("ExAllocatePoolTypeWithTag(type=%u size=%llu tag=%08X) = 0x%08X%s",
             (unsigned)ctx.r3.u32, (unsigned long long)size,
             (unsigned)ctx.r5.u32, (unsigned)ctx.r3.u32,
             p == 0 ? " — HEAP ESGOTADO" : "");
    }
}

void real_ExFreePool(PPCContext& ctx, uint8_t* base) {
    heap().free(ctx.r3.u64);
}

void real_NtAllocateVirtualMemory(PPCContext& ctx, uint8_t* base) {
    // Assinatura REAL do xboxkrnl (idêntica à do Xenia):
    //   (PVOID* BaseAddress, PSIZE_T RegionSize, ULONG AllocationType,
    //    ULONG Protect, BOOLEAN DebugMemory)
    // r5 = tipo: X_MEM_* (0x1000 COMMIT, 0x2000 RESERVE, 0x20000000
    //      LARGE_PAGES 64k, 0x40000000 HEAP, 0x800000 NOZERO …)
    // r6 = proteção (X_PAGE_*: 4 = READWRITE), r7 = debug memory.
    constexpr uint32_t kMemCommit = 0x1000, kMemReserve = 0x2000,
                      kMemReset = 0x80000, kMemNoZero = 0x800000,
                      kMemLargePages = 0x20000000;

    const uint64_t pBase = ctx.r3.u64;   // PVOID* (in/out)
    const uint64_t pSize = ctx.r4.u64;   // PSIZE_T (in/out, 4 bytes no Xenon)
    const uint32_t allocType = (uint32_t)ctx.r5.u64;
    (void)ctx.r6;                        // Protect (nossa memória é RW|X)
    (void)ctx.r7;                        // DebugMemory (devkit) = 0

    if (!validGuest(pBase, 4) || !validGuest(pSize, 4)) {
        ctx.r3.u32 = STATUS_INVALID_PARAMETER;
        return;
    }
    const uint32_t wantBase = g32(base, pBase);
    const int32_t sizeRaw = (int32_t)g32(base, pSize);
    if (sizeRaw == 0 || !(allocType & (kMemCommit | kMemReserve | kMemReset))) {
        ctx.r3.u32 = STATUS_INVALID_PARAMETER;
        return;
    }
    // página 64k com X_MEM_LARGE_PAGES; negativo = tamanho absoluto
    const uint64_t page = (allocType & kMemLargePages) ? 0x10000 : 0x1000;
    uint64_t size = sizeRaw < 0 ? -(uint64_t)sizeRaw : (uint64_t)sizeRaw;
    size = (size + page - 1) & ~(page - 1);

    uint64_t addr = 0;
    if (wantBase != 0) {
        const uint64_t alignedBase = wantBase & ~(page - 1);
        addr = virtWindow().allocAt(alignedBase, size);
        if (!addr && virtWindow().containsRange(alignedBase, size)) {
            // COMMIT dentro de uma RESERVA existente: no kernel real a
            // região reservada passa a committed; no nosso modelo a memória
            // já é acessível — sucesso com a base pedida.
            addr = alignedBase;
        }
    } else {
        addr = virtWindow().alloc(size, page);
    }
    if (!addr) {
        ctx.r3.u32 = STATUS_NO_MEMORY;
        RLOG("NtAllocateVirtualMemory(size=%llu page=%lluk base=%08X "
             "type=%08X) = NO_MEMORY",
             (unsigned long long)size, (unsigned long long)(page >> 10),
             wantBase, allocType);
        return;
    }
    // MEM_COMMIT sem X_MEM_NOZERO entrega memória zerada (kernel real)
    if ((allocType & kMemCommit) && !(allocType & kMemNoZero) &&
        validGuest(addr, 1)) {
        memset(base + (uint32_t)addr, 0, size);
    }
    w32(base, pBase, (uint32_t)addr);
    w32(base, pSize, (uint32_t)size);
    ctx.r3.u32 = STATUS_SUCCESS;
    static Throttle t;
    if (t.shouldLog(24, 512)) {
        RLOG("NtAllocateVirtualMemory(size=%llu type=%08X) = 0x%08X",
             (unsigned long long)size, allocType, (unsigned)addr);
    }
}

void real_NtFreeVirtualMemory(PPCContext& ctx, uint8_t* base) {
    // (PVOID* BaseAddress, PSIZE_T RegionSize, ULONG FreeType, BOOLEAN Debug)
    const uint64_t pBase = ctx.r3.u64;
    if (validGuest(pBase, 4)) {
        const uint64_t addr = g32(base, pBase);
        virtWindow().free(addr);
    }
    ctx.r3.u32 = STATUS_SUCCESS;
}

// NtQueryVirtualMemory(BaseAddress, PMEMORY_BASIC_INFORMATION) — consulta
// REAL do modelo de memória do runtime: imagem+tabela mágica (EXEC), heap
// (RW), janela virtual (blocos comprometidos) e vãos livres. É o que o CRT
// do título usa para escolher onde criar o heap inicial.
void real_NtQueryVirtualMemory(PPCContext& ctx, uint8_t* base) {
    // (PVOID BaseAddress, PMEMORY_BASIC_INFORMATION MemoryInformation)
    const uint64_t addr = ctx.r3.u64;
    const uint64_t pMbi = ctx.r4.u64;
    if (!validGuest(pMbi, 28) || addr < 0x80000000ull ||
        addr >= 0xA0000000ull) {
        ctx.r3.u32 = STATUS_INVALID_PARAMETER;
        return;
    }
    constexpr uint64_t kUserLo = 0x80000000ull, kUserHi = 0xA0000000ull;
    constexpr uint32_t kMemCommit = 0x1000, kMemFree = 0x10000;
    constexpr uint32_t kMemImage = 0x1000000;
    constexpr uint32_t kPageNoAccess = 0x01, kPageReadWrite = 0x04,
                      kPageExecReadWrite = 0x40;

    const uint64_t magicEnd =
        PPC_IMAGE_BASE + PPC_IMAGE_SIZE +
        ((PPC_CODE_SIZE * 2ull + 0xFFFFull) & ~0xFFFFull);
    const uint64_t imageStart = PPC_IMAGE_BASE;
    const uint64_t heapStart = heap().base();
    const uint64_t heapEnd = heapStart ? heapStart + heap().size() : 0;
    const uint64_t virtStart = virtWindow().base();
    const uint64_t virtEnd = virtStart ? virtStart + virtWindow().size() : 0;

    uint64_t rbase = 0, rsize = 0, aBase = 0;
    uint32_t aProtect = kPageNoAccess, state = kMemFree,
             protect = kPageNoAccess, type = 0;

    if (addr >= imageStart && addr < magicEnd) {
        // imagem carregada + tabela mágica de funções
        rbase = imageStart;
        rsize = magicEnd - imageStart;
        aBase = imageStart;
        aProtect = kPageExecReadWrite;
        state = kMemCommit;
        protect = kPageExecReadWrite;
        type = kMemImage;
    } else if (heapStart && addr >= heapStart && addr < heapEnd) {
        rbase = heapStart;
        rsize = heapEnd - heapStart;
        aBase = heapStart;
        aProtect = kPageReadWrite;
        state = kMemCommit;
        protect = kPageReadWrite;
    } else if (virtStart && addr >= virtStart && addr < virtEnd) {
        uint64_t b = 0, s = 0;
        bool committed = false;
        if (!virtWindow().regionAt(addr, b, s, committed)) {
            ctx.r3.u32 = STATUS_INVALID_PARAMETER;
            return;
        }
        rbase = b;
        rsize = s;
        if (committed) {
            aBase = b;
            aProtect = kPageReadWrite;
            state = kMemCommit;
            protect = kPageReadWrite;
        }
    } else {
        // vão livre até a próxima zona comprometida
        uint64_t lo = kUserLo, hi = kUserHi;
        const std::pair<uint64_t, uint64_t> zones[] = {
            {imageStart, magicEnd}, {heapStart, heapEnd},
            {virtStart, virtEnd}};
        for (const auto& z : zones) {
            if (z.second <= addr && z.second > lo) lo = z.second;
            if (z.first > addr && z.first < hi) hi = z.first;
        }
        rbase = lo;
        rsize = hi - lo;
    }

    w32(base, pMbi + 0, (uint32_t)rbase);
    w32(base, pMbi + 4, (uint32_t)aBase);
    w32(base, pMbi + 8, (uint32_t)aProtect);
    w32(base, pMbi + 12, (uint32_t)rsize);
    w32(base, pMbi + 16, (uint32_t)state);
    w32(base, pMbi + 20, (uint32_t)protect);
    w32(base, pMbi + 24, (uint32_t)type);
    ctx.r3.u32 = STATUS_SUCCESS;
}

void real_MmAllocatePhysicalMemoryEx(PPCContext& ctx, uint8_t* base) {
    // (Flags/Type, Size, MinAddr, MaxAddr, Alignment, Tag) — 6 args:
    // r3=tipo, r4=tamanho, r5=min, r6=max, r7=ALINHAMENTO, r8=tag.
    // ALOCAÇÃO FÍSICA REAL: páginas da janela física (alias 0xA0000000+,
    // espelho das páginas RAM flat). Alinhamento real (r7) com clamp sano.
    const uint64_t size = ctx.r4.u64;
    uint64_t align = ctx.r7.u64;
    if (align < 0x1000) align = 0x1000;             // páginas 4k no mínimo
    if (align > 0x1000000) align = 0x1000000;       // sanity: 16 MB max
    const uint64_t addr = physWindow().alloc((size + align - 1) & ~(align - 1));
    ctx.r3.u64 = addr;
    if (!addr) {
        RLOG("MmAllocatePhysicalMemoryEx(type=%u size=%llu min=%08X max=%08X "
             "align=%llu tag=%08X) = NULL (janela física cheia?)",
             (unsigned)ctx.r3.u32, (unsigned long long)size,
             (unsigned)ctx.r5.u32, (unsigned)ctx.r6.u32,
             (unsigned long long)align, (unsigned)ctx.r8.u32);
    }
}

void real_MmFreePhysicalMemory(PPCContext& ctx, uint8_t* base) {
    physWindow().free(ctx.r4.u64);
}

void real_MmGetPhysicalAddress(PPCContext& ctx, uint8_t* base) {
    // (PVOID VirtualAddress) → PHYSICAL_ADDRESS: PA = VA - base da vista.
    // Flat 0x80000000+n e alias 0xA0000000+n referem-se ao mesmo PA n.
    const uint64_t va = ctx.r3.u32;
    if (va >= 0x80000000ull && va < 0xA0000000ull) {
        ctx.r3.u64 = va - 0x80000000ull;
    } else if (va >= 0xA0000000ull && va < 0xC0000000ull) {
        ctx.r3.u64 = va - 0xA0000000ull;
    } else {
        ctx.r3.u64 = 0;
    }
}

void real_MmQueryAllocationSize(PPCContext& ctx, uint8_t* base) {
    const uint64_t addr = ctx.r3.u64;
    const uint64_t sz = virtWindow().blockSize(addr);
    ctx.r3.u64 = sz;
}

void real_MmQueryStatistics(PPCContext& ctx, uint8_t* base) {
    // (PMM_STATISTICS Statistics) — números REAIS do modelo de memória:
    // 512 MB de RAM (janelas flat/física espelhadas) + páginas livres reais.
    const uint64_t pStats = ctx.r3.u32;
    if (validGuest(pStats, 4)) {
        const uint32_t length = g32(base, pStats);
        // MM_STATISTICS: {Length, TotalPhysicalPages, AvailablePhysicalPages,
        //  TotalVirtualBytes, AvailableVirtualBytes, ...}
        const uint32_t totalPages = 0x20000000u >> 12; // 512 MB / 4k
        const uint64_t physFree = physWindow().size() - physWindow().used();
        w32(base, pStats + 4, totalPages);
        if (length >= 8) w32(base, pStats + 8, (uint32_t)(physFree >> 12));
        if (length >= 0x18) {
            w32(base, pStats + 0x10, 0x20000000u);   // TotalVirtualBytes
            w32(base, pStats + 0x14, (uint32_t)physFree); // AvailableVirtual
        }
    }
    ctx.r3.u32 = STATUS_SUCCESS;
}

void real_MmCreateKernelStack(PPCContext& ctx, uint8_t* base) {
    // (&StackBase, Bytes, Rax/Node, Processor) — escreve base em *r3
    const uint64_t bytes = ctx.r4.u64 ? ctx.r4.u64 : 0x4000;
    const uint64_t addr = virtWindow().alloc(bytes);
    if (addr && validGuest(ctx.r3.u64, 4)) w32(base, ctx.r3.u64, (uint32_t)addr);
    ctx.r3.u32 = addr ? STATUS_SUCCESS : STATUS_NO_MEMORY;
    RLOG("MmCreateKernelStack(%lluk) = 0x%08X (%u)",
         (unsigned long long)(bytes >> 10), (unsigned)addr,
         (unsigned)ctx.r3.u32);
}

void real_MmDeleteKernelStack(PPCContext& ctx, uint8_t* base) {
    virtWindow().free(ctx.r3.u64);
}

// --------------------------------------------------------------- tempo

void real_KeQuerySystemTime(PPCContext& ctx, uint8_t* base) {
    if (validGuest(ctx.r3.u64, 8)) w64(base, ctx.r3.u64, systemTime100ns());
}

void real_KeQueryPerformanceFrequency(PPCContext& ctx, uint8_t* base) {
    ctx.r3.u64 = 50000000ull; // 50 MHz do timer Xenon
}

// --------------------------------------------------------------- espera

void real_KeDelayExecutionThread(PPCContext& ctx, uint8_t* base) {
    // (Mode, Alertable, PLARGE_INTEGER Interval) — negativo = relativo,
    // unidades de 100 ns; NULL = kNoTimeout (tratado como espera nula —
    // o kernel real exige intervalo válido; guest nunca passa NULL aqui)
    const uint64_t pInterval = ctx.r5.u64;
    const int64_t val = readTimeout(base, pInterval);
    int64_t ns = 0;
    if (val != kNoTimeout && val < 0) ns = -(val + 1) * 100ull + 100ull;
    else if (val != kNoTimeout && val > 0) {
        // ABSOLUTO (epoch 1601, 100ns): espera o RESTANTE real até lá
        const int64_t now100 = (int64_t)systemTime100ns();
        ns = (val > now100) ? (val - now100) * 100ll : 0;
    }
    static Throttle t;
    if (t.shouldLog(24, 512)) {
        RLOG("KeDelayExecutionThread(%lld 100ns = %.3f ms)", (long long)val,
             ns / 1e6);
    }
    if (ns > 0) {
        // fatia em pedaços curtos para responder a stop
        auto deadline = std::chrono::steady_clock::now() +
                        std::chrono::nanoseconds(ns);
        while (std::chrono::steady_clock::now() < deadline) {
            if (stopRequested()) {
                jmp_buf* env = unwindTarget();
                if (env) longjmp(*env, 1);
                return;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(2));
        }
    }
    ctx.r3.u32 = STATUS_SUCCESS;
}

void real_NtYieldExecution(PPCContext&, uint8_t*) {
    std::this_thread::yield();
}

// -------------------------------------------------------- sincronização

void real_KeSetEvent(PPCContext& ctx, uint8_t* base) {
    ctx.r3.u32 = (uint32_t)setEvent(ev(base, ctx.r3.u64, true));
}

void real_KeResetEvent(PPCContext& ctx, uint8_t* base) {
    ctx.r3.u32 = (uint32_t)resetEvent(ev(base, ctx.r3.u64, true));
}

void real_KeInitializeMutant(PPCContext& ctx, uint8_t* base) {
    // (Mutant, InitialOwner)
    Waitable* w = waitable(ctx.r3.u64, true, WaitKind::Mutant);
    if (w) {
        std::lock_guard<std::mutex> lk(waitMutex());
        w->signaled = ctx.r4.u64 == 0; // sem dono = livre
        w->kind = WaitKind::Mutant;
    }
    ctx.r3.u32 = STATUS_SUCCESS;
}

void real_KeReleaseMutant(PPCContext& ctx, uint8_t* base) {
    // (Mutant, Priority, Abandoned, Wait)
    bool abandoned = false;
    releaseMutant(waitable(ctx.r3.u64, true, WaitKind::Mutant), abandoned);
    ctx.r3.u32 = STATUS_SUCCESS;
}

// ---- semáforos/timers do kernel (Ke* por ENDEREÇO de objeto guest) ----
// Necessários aos módulos secundários recompilados (XMediaFacade/SpeechFacade
// importam KeInitializeSemaphore/KeReleaseSemaphore/Ke*Timer); o par
// NtCreateSemaphore/NtReleaseSemaphore (por HANDLE) já existia.

void real_KeInitializeSemaphore(PPCContext& ctx, uint8_t* base) {
    // (Semaphore, Count, Limit) — objeto guest por endereço
    Waitable* w = waitable(ctx.r3.u64, true, WaitKind::Semaphore);
    if (w) {
        std::lock_guard<std::mutex> lk(waitMutex());
        w->kind = WaitKind::Semaphore;
        w->count = (long)(int32_t)ctx.r4.u32;
        w->maxCount = (long)(int32_t)ctx.r5.u32;
        if (w->maxCount <= 0) w->maxCount = 1;
        w->signaled = w->count > 0;
        w->autoReset = false; // consumo decrementa (tratado no consume)
    }
    ctx.r3.u32 = STATUS_SUCCESS;
}

void real_KeReleaseSemaphore(PPCContext& ctx, uint8_t* base) {
    // (Semaphore, PriorityIncrement, Adjustment, Wait) — Wait é dica do
    // chamador ("vou esperar em seguida"), sem efeito próprio no NT
    Waitable* w = waitable(ctx.r3.u64, true, WaitKind::Semaphore);
    releaseSemaphore(w, (long)(int32_t)ctx.r5.u32);
    ctx.r3.u32 = STATUS_SUCCESS;
}

void real_KeInitializeTimerEx(PPCContext& ctx, uint8_t* base) {
    // (Timer, Type) — 0 = NotificationTimer (manual-reset),
    // 1 = SynchronizationTimer (auto-reset)
    Waitable* w = waitable(ctx.r3.u64, true, WaitKind::Timer);
    if (w) {
        std::lock_guard<std::mutex> lk(waitMutex());
        w->kind = WaitKind::Timer;
        w->autoReset = ctx.r4.u64 == 1;
        w->signaled = false;
        w->dueAbs100ns = 0;
    }
    ctx.r3.u32 = STATUS_SUCCESS;
}

void real_KeSetTimer(PPCContext& ctx, uint8_t* base) {
    // (Timer, DueTime [valor 64-bit, 100ns; <0 = relativo], Dpc)
    Waitable* w = waitable(ctx.r3.u64, true, WaitKind::Timer);
    const bool wasPending = setTimerDue(w, (int64_t)ctx.r4.u64);
    if (ctx.r5.u64 != 0) {
        static Throttle t;
        if (t.shouldLog(4, 64)) {
            RLOG("KeSetTimer(dpc=0x%08X) — DPC não despachado (o vencimento "
                 "sinaliza o timer p/ KeWait: semântica real do objeto)",
                 (unsigned)ctx.r5.u32);
        }
    }
    ctx.r3.u32 = wasPending ? 1 : 0;
}

void real_KeCancelTimer(PPCContext& ctx, uint8_t* base) {
    // (Timer) — retorna BOOL "estava pendente"
    Waitable* w = waitable(ctx.r3.u64, true, WaitKind::Timer);
    ctx.r3.u32 = cancelTimer(w) ? 1 : 0;
}

void real_KeWaitForSingleObject(PPCContext& ctx, uint8_t* base) {
    // (Object, Reason, Mode, Alertable, PLARGE_INTEGER Timeout)
    const uint64_t obj = ctx.r3.u64;
    const bool alertable = ctx.r6.u64 != 0;
    Waitable* w = obj ? waitable(obj, true, WaitKind::Event) : nullptr;
    ctx.r3.u32 = waitOne(base, w, ctx.r7.u64, alertable);
}

void real_KeWaitForMultipleObjects(PPCContext& ctx, uint8_t* base) {
    // (Count, Objects[], WaitType, Reason, Mode, Alertable, Timeout, Block)
    const uint32_t count = (uint32_t)ctx.r3.u64;
    const uint64_t pObjs = ctx.r4.u64;
    const bool waitAny = ctx.r5.u64 == 1; // WAIT_ALL=0, WAIT_ANY=1
    const bool alertable = ctx.r8.u64 != 0;
    const uint64_t pTimeout = ctx.r9.u64;

    if (count == 0 || count > 64 || !validGuest(pObjs, 4ull * count)) {
        ctx.r3.u32 = STATUS_INVALID_PARAMETER;
        return;
    }
    Waitable* objs[64];
    for (uint32_t i = 0; i < count; ++i) {
        const uint64_t obj = g32(base, pObjs + 4ull * i);
        objs[i] = obj ? waitable(obj, true, WaitKind::Event) : nullptr;
        if (!objs[i]) {
            ctx.r3.u32 = STATUS_INVALID_HANDLE;
            return;
        }
    }
    ctx.r3.u32 = waitForMultiple(objs, count, waitAny,
                                 readTimeout(base, pTimeout), alertable);
}

void real_NtCreateEvent(PPCContext& ctx, uint8_t* base) {
    // (&Handle, ObjectAttributes, EventType, InitialState)
    auto* w = new Waitable();
    w->kind = WaitKind::Event;
    w->autoReset = ctx.r5.u64 == 1; // SynchronizationEvent
    w->signaled = ctx.r6.u64 != 0;
    const uint32_t h = registerHandle(w);
    if (validGuest(ctx.r3.u64, 4)) w32(base, ctx.r3.u64, h);
    ctx.r3.u32 = STATUS_SUCCESS;
}

void real_NtSetEvent(PPCContext& ctx, uint8_t* base) {
    const uint32_t h = (uint32_t)ctx.r3.u64;
    const long prev = setEvent(waitableByHandle(h));
    if (validGuest(ctx.r4.u64, 4)) w32(base, ctx.r4.u64, (uint32_t)prev);
    ctx.r3.u32 = STATUS_SUCCESS;
}

void real_NtClearEvent(PPCContext& ctx, uint8_t* base) {
    resetEvent(waitableByHandle((uint32_t)ctx.r3.u64));
    ctx.r3.u32 = STATUS_SUCCESS;
}

void real_NtCreateSemaphore(PPCContext& ctx, uint8_t* base) {
    // (&Handle, ObjectAttributes, InitialCount, MaximumCount)
    auto* w = new Waitable();
    w->kind = WaitKind::Semaphore;
    w->count = (long)(int32_t)ctx.r5.u32;
    w->maxCount = (long)(int32_t)ctx.r6.u32;
    w->signaled = w->count > 0;
    const uint32_t h = registerHandle(w);
    if (validGuest(ctx.r3.u64, 4)) w32(base, ctx.r3.u64, h);
    ctx.r3.u32 = STATUS_SUCCESS;
}

void real_NtReleaseSemaphore(PPCContext& ctx, uint8_t* base) {
    const long prev = releaseSemaphore(waitableByHandle((uint32_t)ctx.r3.u64),
                                       (long)(int32_t)ctx.r4.u32);
    if (validGuest(ctx.r5.u64, 4)) w32(base, ctx.r5.u64, (uint32_t)prev);
    ctx.r3.u32 = STATUS_SUCCESS;
}

void real_NtCreateMutant(PPCContext& ctx, uint8_t* base) {
    // (&Handle, ObjectAttributes, InitialOwner)
    auto* w = new Waitable();
    w->kind = WaitKind::Mutant;
    w->signaled = ctx.r5.u64 == 0;
    const uint32_t h = registerHandle(w);
    if (validGuest(ctx.r3.u64, 4)) w32(base, ctx.r3.u64, h);
    ctx.r3.u32 = STATUS_SUCCESS;
}

void real_NtReleaseMutant(PPCContext& ctx, uint8_t* base) {
    bool abandoned = false;
    releaseMutant(waitableByHandle((uint32_t)ctx.r3.u64), abandoned);
    if (validGuest(ctx.r4.u64, 4)) w32(base, ctx.r4.u64, 0);
    ctx.r3.u32 = STATUS_SUCCESS;
}

void real_NtWaitForSingleObjectEx(PPCContext& ctx, uint8_t* base) {
    // (Handle, WaitMode, Alertable, Timeout)
    Waitable* w = waitableByHandle((uint32_t)ctx.r3.u64);
    ctx.r3.u32 = waitOne(base, w, ctx.r6.u64, ctx.r5.u64 != 0);
}

void real_NtWaitForMultipleObjectsEx(PPCContext& ctx, uint8_t* base) {
    // (Count, Handles[], WaitType, WaitMode, Alertable, Timeout)
    const uint32_t count = (uint32_t)ctx.r3.u64;
    const uint64_t pHandles = ctx.r4.u64;
    const bool waitAny = ctx.r5.u64 == 1; // NT: 1 = WaitAny
    const bool alertable = ctx.r7.u64 != 0;
    const uint64_t pTimeout = ctx.r8.u64;

    if (count == 0 || count > 64 || !validGuest(pHandles, 4ull * count)) {
        ctx.r3.u32 = STATUS_INVALID_PARAMETER;
        return;
    }
    Waitable* objs[64];
    for (uint32_t i = 0; i < count; ++i) {
        objs[i] = waitableByHandle(g32(base, pHandles + 4ull * i));
        if (!objs[i]) {
            ctx.r3.u32 = STATUS_INVALID_HANDLE;
            return;
        }
    }
    ctx.r3.u32 = waitForMultiple(objs, count, waitAny,
                                 readTimeout(base, pTimeout), alertable);
}

void real_NtSignalAndWaitForSingleObjectEx(PPCContext& ctx, uint8_t* base) {
    // (HandleToSignal, HandleToWait, WaitMode, Alertable, Timeout)
    setEvent(waitableByHandle((uint32_t)ctx.r3.u64));
    Waitable* w = waitableByHandle((uint32_t)ctx.r4.u64);
    ctx.r3.u32 = waitOne(base, w, ctx.r7.u64, ctx.r6.u64 != 0);
}

void real_NtClose(PPCContext& ctx, uint8_t* base) {
    // handles Nt podem referenciar waitables OU arquivos abertos
    const uint32_t handle = ctx.r3.u32;
    if (handle >= 0x60000000u && handle < 0x70000000u) {
        fileClose(handle);
    } else {
        closeHandle(handle);
    }
    ctx.r3.u32 = STATUS_SUCCESS;
}

// ----------------------------------------------------- critical sections
//
// SEMÂNTICA REAL do kernel do Xbox 360: RTL_CRITICAL_SECTION é uma estrutura
// de 0x18 bytes EM MEMÓRIA GUEST (não é objeto de kernel). Layout extraído do
// binário do FH2 (critsec estática em 0x833DB4E0, confirmada com dump):
//   +0x00 flags (0x01000400)        +0x04 unused
//   +0x08 lista de espera Flink     +0x0C lista de espera Blink
//        (vazia = aponta para si mesma)
//   +0x10 lock_count (s32; -1 = livre, >=0 = travado + nº de waiters)
//   +0x14 owning_thread (id do kernel da thread dona; 0 = nenhuma)
//
// Enter/Leave/TryEnter operam sobre esses campos com a MESMA semântica do
// ntdll (InterlockedIncrement/CAS): funciona para critsecs estáticas da
// imagem (já nascem livres no .data), critsecs em heap (RtlInitialize...),
// reentrância e contenção entre threads guest.

// Leitura/escrita dos campos (big-endian) — chamadas SEMPRE sob waitMutex().
inline int32_t critsecLock(uint8_t* base, uint64_t cs) {
    return (int32_t)g32(base, cs + 0x10);
}
inline uint32_t critsecOwner(uint8_t* base, uint64_t cs) {
    return g32(base, cs + 0x14);
}
inline void critsecSetLock(uint8_t* base, uint64_t cs, int32_t v) {
    w32(base, cs + 0x10, (uint32_t)v);
}
inline void critsecSetOwner(uint8_t* base, uint64_t cs, uint32_t v) {
    w32(base, cs + 0x14, v);
}

void real_RtlInitializeCriticalSection(PPCContext& ctx, uint8_t* base) {
    // Endereços guest chegam sign-extendidos (lis/addi do PPC) — o kernel
    // real opera em 32 bits: mascarar ANTES de usar.
    const uint64_t cs = ctx.r3.u32;
    if (validGuest(cs, 0x18)) {
        // Estado inicial IDÊNTICO ao das critsecs estáticas da imagem:
        // flags 0x01000400, lista vazia apontando para si, lock_count -1.
        w32(base, cs + 0x00, 0x01000400);
        w32(base, cs + 0x04, 0);
        w32(base, cs + 0x08, (uint32_t)(cs + 8));
        w32(base, cs + 0x0C, (uint32_t)(cs + 8));
        critsecSetLock(base, cs, -1);
        critsecSetOwner(base, cs, 0);
    }
    ctx.r3.u32 = STATUS_SUCCESS;
}

void real_RtlInitializeCriticalSectionAndSpinCount(PPCContext& ctx, uint8_t* base) {
    // (PRTL_CRITICAL_SECTION, ULONG SpinCount): inicialização idêntica; o
    // spin é um detalhe de implementação do kernel real (nosso protocolo é
    // cv-based) — a semântica externa observável é a mesma.
    real_RtlInitializeCriticalSection(ctx, base);
}

void real_RtlEnterCriticalSection(PPCContext& ctx, uint8_t* base) {
    const uint64_t cs = ctx.r3.u32; // 32-bit: endereços vêm sign-extendidos
    if (!validGuest(cs, 0x18)) {
        ctx.r3.u32 = STATUS_INVALID_PARAMETER;
        return;
    }
    const uint32_t self = currentThreadId();
    std::unique_lock<std::mutex> lk(waitMutex());
    for (;;) {
        const int32_t lock = critsecLock(base, cs);
        if (lock < 0) {
            // livre → adquire (fast path do kernel real: -1 → 0)
            critsecSetLock(base, cs, 0);
            critsecSetOwner(base, cs, self);
            return;
        }
        if (self != 0 && critsecOwner(base, cs) == self) {
            // reentrância: profundidade implícita no lock_count
            critsecSetLock(base, cs, lock + 1);
            return;
        }
        // contenção: espera o Leave notificar (cv do kernel). O predicate
        // relê a memória do guest a cada avaliação — sem wakeup perdido.
        waitCv().wait(lk, [&] {
            return critsecLock(base, cs) < 0 ||
                   (self != 0 && critsecOwner(base, cs) == self) ||
                   stopRequested();
        });
        if (stopRequested()) {
            lk.unlock();
            jmp_buf* env = unwindTarget();
            if (env) longjmp(*env, 1);
            return; // stop sem alvo de unwind: não trava o encerramento
        }
    }
}

void real_RtlLeaveCriticalSection(PPCContext& ctx, uint8_t* base) {
    const uint64_t cs = ctx.r3.u32; // 32-bit: endereços vêm sign-extendidos
    if (!validGuest(cs, 0x18)) {
        ctx.r3.u32 = STATUS_INVALID_PARAMETER;
        return;
    }
    {
        std::lock_guard<std::mutex> lk(waitMutex());
        const int32_t lock = critsecLock(base, cs);
        if (lock > 0) {
            // soltando um nível de reentrância
            critsecSetLock(base, cs, lock - 1);
        } else {
            // soltando de vez: -1 → 0... (0 → -1) e acorda UM esperando
            critsecSetOwner(base, cs, 0);
            critsecSetLock(base, cs, -1);
        }
    }
    waitCv().notify_all();
    ctx.r3.u32 = STATUS_SUCCESS;
}

void real_RtlTryEnterCriticalSection(PPCContext& ctx, uint8_t* base) {
    const uint64_t cs = ctx.r3.u32; // 32-bit: endereços vêm sign-extendidos
    ctx.r3.u32 = 0; // FALSE
    if (!validGuest(cs, 0x18)) return;
    const uint32_t self = currentThreadId();
    std::lock_guard<std::mutex> lk(waitMutex());
    const int32_t lock = critsecLock(base, cs);
    if (lock < 0) {
        critsecSetLock(base, cs, 0);
        critsecSetOwner(base, cs, self);
        ctx.r3.u32 = 1; // TRUE
        return;
    }
    if (self != 0 && critsecOwner(base, cs) == self) {
        critsecSetLock(base, cs, lock + 1);
        ctx.r3.u32 = 1; // TRUE
    }
}

// ------------------------------------------------------------ TLS kernel

void real_KeTlsAlloc(PPCContext& ctx, uint8_t* base) {
    ctx.r3.u32 = tlsAlloc();
}

void real_KeTlsFree(PPCContext& ctx, uint8_t* base) {
    tlsFree((uint32_t)ctx.r3.u64);
    ctx.r3.u32 = STATUS_SUCCESS;
}

void real_KeTlsGetValue(PPCContext& ctx, uint8_t* base) {
    ctx.r3.u64 = (uint64_t)(uintptr_t)tlsGetValue((uint32_t)ctx.r3.u64);
}

void real_KeTlsSetValue(PPCContext& ctx, uint8_t* base) {
    ctx.r3.u32 = tlsSetValue((uint32_t)ctx.r3.u64,
                             (void*)(uintptr_t)ctx.r4.u64) ? 1 : 0;
}

// ------------------------------------------------------------- threads

void real_ExCreateThread(PPCContext& ctx, uint8_t* base) {
    // (&Handle, StackSize, &ThreadId, XapiThreadStart, StartAddress,
    //  StartContext, Flags)
    uint32_t id = 0;
    const uint32_t h = createThread(ctx.r4.u64, ctx.r7.u64, ctx.r8.u64, &id);
    if (validGuest(ctx.r3.u64, 4)) w32(base, ctx.r3.u64, h);
    if (validGuest(ctx.r5.u64, 4)) w32(base, ctx.r5.u64, id);
    ctx.r3.u32 = STATUS_SUCCESS;
    RLOG("ExCreateThread(handle=0x%08X stack=%lluk start=0x%08X ctx=0x%08X)",
         h, (unsigned long long)(ctx.r4.u64 >> 10), (unsigned)ctx.r7.u32,
         (unsigned)ctx.r8.u32);
}

void real_ExTerminateThread(PPCContext& ctx, uint8_t* base) {
    // longjmp para o corpo da thread (código 2 = término limpo)
    traceDump("ExTerminateThread");
    jmp_buf* env = unwindTarget();
    if (env) longjmp(*env, 2);
    ctx.r3.u32 = STATUS_SUCCESS;
}

void real_KeResumeThread(PPCContext& ctx, uint8_t* base) {
    const uint32_t h = (uint32_t)ctx.r3.u64;
    const bool ok = resumeThread(h);
    ctx.r3.u32 = ok ? STATUS_SUCCESS : STATUS_INVALID_HANDLE;
    if (!ok) RLOG("KeResumeThread(0x%08X): handle desconhecido", h);
}

// NtResumeThread/NtSuspendThread: contagem de suspensão REAL (a thread do
// jogo é criada SUSPENSA e só executa após a contagem chegar a 0).
void real_NtResumeThread(PPCContext& ctx, uint8_t* base) {
    const uint32_t h = ctx.r3.u32;
    GuestThread* t = findThread(h);
    if (!t) {
        ctx.r3.u32 = STATUS_INVALID_HANDLE;
        return;
    }
    // PULONG PreviousSuspendCount (r4)
    const uint32_t prev = t->suspendCount.fetch_sub(1);
    if (validGuest(ctx.r4.u32, 4)) w32(base, ctx.r4.u32, prev);
    if (prev <= 1) {
        std::lock_guard<std::mutex> lk(t->startM);
        t->started = true;
        t->startCv.notify_all();
    }
    ctx.r3.u32 = STATUS_SUCCESS;
    static Throttle t1;
    if (t1.shouldLog(8, 256)) {
        RLOG("NtResumeThread(0x%08X) prev=%u", h, prev);
    }
}

void real_NtSuspendThread(PPCContext& ctx, uint8_t* base) {
    const uint32_t h = ctx.r3.u32;
    GuestThread* t = findThread(h);
    if (!t) {
        ctx.r3.u32 = STATUS_INVALID_HANDLE;
        return;
    }
    const uint32_t prev = t->suspendCount.fetch_add(1);
    if (validGuest(ctx.r4.u32, 4)) w32(base, ctx.r4.u32, prev);
    ctx.r3.u32 = STATUS_SUCCESS;
}

void real_KeSetAffinityThread(PPCContext& ctx, uint8_t* base) {
    // (HANDLE, KAFFINITY affinity, PKAFFINITY PreviousAffinity) — o Xenon tem
    // 6 cores; registramos a afinidade pedida (efeito de agendamento é do
    // host) e devolvemos a anterior real (todas as cores: 0x3F).
    if (validGuest(ctx.r5.u32, 4)) w32(base, ctx.r5.u32, 0x3F);
    ctx.r3.u32 = STATUS_SUCCESS;
}

void real_KeSetBasePriorityThread(PPCContext& ctx, uint8_t* base) {
    // (HANDLE, LONG Priority, PLONG PreviousPriority)
    if (validGuest(ctx.r5.u32, 4)) w32(base, ctx.r5.u32, 2);
    ctx.r3.u32 = STATUS_SUCCESS;
}

void real_ObReferenceObjectByHandle(PPCContext& ctx, uint8_t* base) {
    // (HANDLE, ACCESS_MASK, ..., PVOID* Object) — devolve um ponteiro-opaco
    // ESTÁVEL em memória guest REAL por handle (o guest trata como opaco e o
    // repassa de volta ao kernel; não há layout a preservar).
    const uint32_t h = ctx.r3.u32;
    const uint64_t pOut = ctx.r5.u32;
    if (!validGuest(pOut, 4)) {
        ctx.r3.u32 = STATUS_INVALID_PARAMETER;
        return;
    }
    // bloco por handle: estável enquanto o objeto existir
    uint64_t block;
    {
        std::lock_guard<std::mutex> lk(g_obM);
        uint64_t& b = g_obBlocks[h];
        if (b == 0) b = heap().alloc(0x200, 32);
        block = b;
    }
    w32(base, pOut, (uint32_t)block);
    ctx.r3.u32 = STATUS_SUCCESS;
}

void real_ObDereferenceObject(PPCContext& ctx, uint8_t* base) {
    ctx.r3.u32 = STATUS_SUCCESS; // contagem interna — nada visível ao guest
}

void real_XGetAVPack(PPCContext& ctx, uint8_t* base) {
    // Valor real de console com HDMI (o jogo usa p/ configurar o display)
    ctx.r3.u32 = 6; // XEX_AV_PACK_HDMI
}

void real_ExGetXConfigSetting(PPCContext& ctx, uint8_t* base) {
    // (ULONG Category, ULONG Setting, PVOID Buffer, ULONG Size, PULONG pcbOut)
    const uint64_t buffer = ctx.r5.u32;
    const uint32_t size = ctx.r6.u32;
    const uint64_t pcbOut = ctx.r7.u32;
    if (!validGuest(buffer, size) || !validGuest(pcbOut, 4)) {
        ctx.r3.u32 = STATUS_INVALID_PARAMETER;
        return;
    }
    // configurações zeradas = defaults de fábrica (vídeo: auto/720p; sem
    // perfil, sem rede configurada). O jogo prossegue com o padrão.
    memset(base + buffer, 0, size);
    w32(base, pcbOut, size);
    ctx.r3.u32 = STATUS_SUCCESS;
}

void real_KeGetCurrentThread(PPCContext& ctx, uint8_t* base) {
    // Ponteiro-opaco estável por thread em memória guest REAL (o guest pode
    // escrever campos do KTHREAD sem corromper nada de terceiros).
    if (t_kthreadBlock == 0) t_kthreadBlock = heap().alloc(0x200, 64);
    ctx.r3.u64 = t_kthreadBlock;
}

void real_KeGetCurrentProcessType(PPCContext& ctx, uint8_t* base) {
    ctx.r3.u32 = 1; // processo "title" (jogo)
}

void real_KeSetCurrentProcessType(PPCContext& ctx, uint8_t* base) {
    RLOG("KeSetCurrentProcessType(%u)", (unsigned)ctx.r3.u32);
}

// ----------------------------------------------------------- fatal paths

void real_KeBugCheck(PPCContext& ctx, uint8_t* base) {
    RLOG("KeBugCheck(code=0x%08X) — bugcheck REAL do guest",
         (unsigned)ctx.r3.u32);
    __android_log_print(ANDROID_LOG_ERROR, "FH2/KRN",
                        "guest BUGCHECK 0x%08X", (unsigned)ctx.r3.u32);
    traceDump("KeBugCheck");
    jmp_buf* env = unwindTarget();
    if (env) longjmp(*env, 3);
}

void real_KeBugCheckEx(PPCContext& ctx, uint8_t* base) {
    RLOG("KeBugCheckEx(code=0x%08X, %016llX, %016llX, %016llX) — bugcheck REAL",
         (unsigned)ctx.r3.u32, (unsigned long long)ctx.r4.u64,
         (unsigned long long)ctx.r5.u64, (unsigned long long)ctx.r6.u64);
    traceDump("KeBugCheckEx");
    jmp_buf* env = unwindTarget();
    if (env) longjmp(*env, 3);
}

void real_HalReturnToFirmware(PPCContext& ctx, uint8_t* base) {
    RLOG("HalReturnToFirmware(%u) — guest pediu desligamento/reboot",
         (unsigned)ctx.r3.u32);
    // Despejo das últimas chamadas HLE: mostra a decisão REAL que fez o
    // guest encerrar (ex.: heap inicial do CRT = 0 → HalReturnToFirmware(1)).
    traceDump("HalReturnToFirmware");
    requestStop();
    jmp_buf* env = unwindTarget();
    if (env) longjmp(*env, 2);
}

// ------------------------------------------------------------- Rtl misc

void real_RtlCompareMemory(PPCContext& ctx, uint8_t* base) {
    // (Source1, Source2, Length) → bytes iguais até o 1º mismatch
    const uint64_t a = ctx.r3.u64, b = ctx.r4.u64, len = ctx.r5.u64;
    uint64_t n = 0;
    if (validGuest(a, 1) && validGuest(b, 1)) {
        // 32-bit EA + limite REAL pela memória mapeada (len do guest pode ser
        // lixo sign-extendido — nunca ler além da RAM guest)
        const uint8_t* pa = base + (uint32_t)a;
        const uint8_t* pb = base + (uint32_t)b;
        const uint64_t aLeft = guestMemBytes() - (uint32_t)a;
        const uint64_t bLeft = guestMemBytes() - (uint32_t)b;
        const uint64_t lim = std::min(len, std::min(aLeft, bLeft));
        while (n < lim && pa[n] == pb[n]) ++n;
    }
    ctx.r3.u64 = n;
}

void real_RtlCompareMemoryUlong(PPCContext& ctx, uint8_t* base) {
    // (Source, Length, Pattern) → bytes (múltiplos de 4) que casam
    const uint64_t a = ctx.r3.u64, len = ctx.r4.u64;
    const uint32_t pat = (uint32_t)ctx.r5.u64;
    uint64_t n = 0;
    if (validGuest(a, 1)) {
        while (n + 4 <= len && g32(base, a + n) == pat) n += 4;
    }
    ctx.r3.u64 = n;
}

void real_RtlFillMemoryUlong(PPCContext& ctx, uint8_t* base) {
    // (Destination, Length, Pattern)
    const uint64_t a = ctx.r3.u64, len = ctx.r4.u64;
    const uint32_t pat = (uint32_t)ctx.r5.u64;
    if (validGuest(a, 1)) {
        for (uint64_t off = 0; off + 4 <= len; off += 4) {
            w32(base, a + off, pat);
        }
    }
    ctx.r3.u32 = STATUS_SUCCESS;
}

void real_RtlTimeFieldsToTime(PPCContext& ctx, uint8_t* base) {
    // (PTIME_FIELDS, PLARGE_INTEGER) — TIME_FIELDS: 8 × u16 BE
    const uint64_t tf = ctx.r3.u64;
    if (!validGuest(tf, 16)) {
        ctx.r3.u64 = 0;
        return;
    }
    // TIME_FIELDS: 8 × u16 BE (Year, Month, Day, Hour, Minute, Second, Ms, Wday)
    const uint32_t ym = g32(base, tf);
    const uint32_t dhm = g32(base, tf + 4);
    const uint32_t smw = g32(base, tf + 8);
    const int year = (int)(ym >> 16), month = (int)(ym & 0xFFFF);
    const int day = (int)(dhm >> 16), hour = (int)(dhm & 0xFFFF);
    const int minute = (int)(smw >> 16), second = (int)(smw & 0xFFFF);
    const uint16_t ms = (uint16_t)(g32(base, tf + 12) >> 16);

    std::tm t{};
    t.tm_year = year - 1900;
    t.tm_mon = month - 1;
    t.tm_mday = day;
    t.tm_hour = hour;
    t.tm_min = minute;
    t.tm_sec = second;
#ifdef _WIN32
    const time_t tt = _mkgmtime(&t);
#else
    const time_t tt = timegm(&t);
#endif
    uint64_t ft = (uint64_t)tt * 10000000ull + (uint64_t)ms * 10000ull +
                  116444736000000000ull;
    if (validGuest(ctx.r4.u64, 8)) w64(base, ctx.r4.u64, ft);
    ctx.r3.u64 = ft;
}

void real_RtlTimeToTimeFields(PPCContext& ctx, uint8_t* base) {
    const uint64_t ft = ctx.r3.u64;
    const uint64_t pTf = ctx.r4.u64;
    if (!validGuest(pTf, 16)) return;
    const uint64_t since1970 = (ft - 116444736000000000ull) / 10000000ull;
    const uint64_t msRest = (ft - 116444736000000000ull) % 10000000ull / 10000ull;
    std::tm t{};
#ifdef _WIN32
    gmtime_s(&t, (const time_t*)&since1970);
#else
    gmtime_r((const time_t*)&since1970, &t);
#endif
    const uint16_t y = (uint16_t)(t.tm_year + 1900);
    const uint16_t mon = (uint16_t)(t.tm_mon + 1);
    const uint16_t day = (uint16_t)t.tm_mday, hour = (uint16_t)t.tm_hour;
    const uint16_t min = (uint16_t)t.tm_min, sec = (uint16_t)t.tm_sec;
    const uint16_t wd = (uint16_t)t.tm_wday, ms = (uint16_t)msRest;
    w32(base, pTf, ((uint32_t)y << 16) | mon);
    w32(base, pTf + 4, ((uint32_t)day << 16) | hour);
    w32(base, pTf + 8, ((uint32_t)min << 16) | sec);
    w32(base, pTf + 12, ((uint32_t)ms << 16) | wd);
}

void real_RtlInitAnsiString(PPCContext& ctx, uint8_t* base) {
    // (PANSI_STRING out, LPCSTR source)
    const uint64_t out = ctx.r3.u64, src = ctx.r4.u64;
    const std::string s = src ? readGuestCString(src) : std::string();
    if (validGuest(out, 8)) {
        w32(base, out, ((uint32_t)s.size() << 16) | (uint32_t)s.size());
        w32(base, out + 4, (uint32_t)src);
    }
}

void real_RtlInitUnicodeString(PPCContext& ctx, uint8_t* base) {
    const uint64_t out = ctx.r3.u64, src = ctx.r4.u64;
    // comprimento em BYTES (UTF-16, 2 bytes/char)
    uint32_t bytes = 0;
    if (src) {
        while (validGuest(src + bytes, 2) && bytes < 8192) {
            const uint32_t v = g32(base, src + bytes);
            if ((v >> 16) == 0) break; // terminador BE
            bytes += 2;
        }
    }
    if (validGuest(out, 8)) {
        w32(base, out, ((uint32_t)bytes << 16) | (uint32_t)bytes);
        w32(base, out + 4, (uint32_t)src);
    }
}

void real_RtlMultiByteToUnicodeN(PPCContext& ctx, uint8_t* base) {
    // (UnicodeString dst, MaxBytes, &ResultLen, AnsiString src, BytesIn)
    const uint64_t uDst = ctx.r3.u64;
    const uint64_t maxU = ctx.r4.u64;
    const uint64_t pRes = ctx.r5.u64;
    const uint64_t aSrc = ctx.r6.u64;
    const uint64_t bytesIn = ctx.r7.u64;
    uint32_t out = 0;
    if (validGuest(uDst, 1) && validGuest(aSrc, 1)) {
        const uint64_t lim = std::min<uint64_t>(maxU / 2, bytesIn);
        for (uint64_t i = 0; i < lim; ++i) {
            const uint8_t c = base[aSrc + i];
            w32(base, uDst + 2 * i, ((uint32_t)c) << 16); // u16 BE = 00,c
            out += 2;
        }
    }
    if (validGuest(pRes, 4)) w32(base, pRes, out);
    ctx.r3.u32 = STATUS_SUCCESS;
}

void real_RtlUnicodeToMultiByteN(PPCContext& ctx, uint8_t* base) {
    // (AnsiString, MaxBytes, &ResultLen, UnicodeString, BytesInUnicode)
    const uint64_t aDst = ctx.r3.u64;
    const uint64_t maxA = ctx.r4.u64;
    const uint64_t pRes = ctx.r5.u64;
    const uint64_t uSrc = ctx.r6.u64;
    const uint64_t bytesIn = ctx.r7.u64;
    uint32_t out = 0;
    if (validGuest(aDst, 1) && validGuest(uSrc, 2)) {
        const uint64_t chars = std::min<uint64_t>(bytesIn / 2, maxA);
        for (uint64_t i = 0; i < chars; ++i) {
            const uint32_t pair = g32(base, uSrc + 2 * i);
            const uint16_t wc = (uint16_t)(pair >> 16);
            base[aDst + i] = wc < 0x80 ? (uint8_t)wc : '?';
            ++out;
        }
    }
    if (validGuest(pRes, 4)) w32(base, pRes, out);
    ctx.r3.u32 = STATUS_SUCCESS;
}

void real_RtlUpcaseUnicodeChar(PPCContext& ctx, uint8_t* base) {
    const uint16_t wc = (uint16_t)ctx.r3.u64;
    ctx.r3.u64 = (wc >= 'a' && wc <= 'z') ? wc - 0x20 : wc;
}

void real_RtlUnicodeStringToAnsiString(PPCContext& ctx, uint8_t* base) {
    // (PANSI_STRING out, PCUNICODE_STRING in, BOOLEAN alloc)
    const uint64_t out = ctx.r3.u64, in = ctx.r4.u64;
    if (validGuest(in, 8)) {
        const uint32_t lenBytes = g32(base, in) >> 16;
        const uint64_t buf = g32(base, in + 4);
        std::string s;
        if (validGuest(buf, 2)) {
            for (uint32_t i = 0; i < lenBytes / 2; ++i) {
                const uint16_t wc = (uint16_t)(g32(base, buf + 2 * i) >> 16);
                s.push_back(wc < 0x80 ? (char)wc : '?');
            }
        }
        // escreve no ANSI_STRING (sem alocação guest real — usa o buffer do
        // caller quando presente; alocação via heap quando alloc=1)
        uint64_t dstBuf = 0;
        if (ctx.r5.u64 != 0) {
            dstBuf = heap().alloc(s.size() + 1, 4);
            if (dstBuf && validGuest(dstBuf, s.size() + 1)) {
                memcpy(base + dstBuf, s.c_str(), s.size() + 1);
            }
        } else if (validGuest(out, 8)) {
            dstBuf = g32(base, out + 4); // buffer pré-alocado pelo caller
        }
        if (validGuest(out, 8) && dstBuf) {
            w32(base, out, ((uint32_t)s.size() << 16) | (uint32_t)s.size());
            w32(base, out + 4, (uint32_t)dstBuf);
        }
        if (dstBuf && validGuest(dstBuf, s.size() + 1)) {
            memcpy(base + dstBuf, s.c_str(), s.size() + 1);
        }
    }
    ctx.r3.u32 = STATUS_SUCCESS;
}

void real_RtlFreeAnsiString(PPCContext& ctx, uint8_t* base) {
    // buffer alocado por nós (alloc=1) mora no heap guest
    if (validGuest(ctx.r3.u64, 8)) heap().free(g32(base, ctx.r3.u64 + 4));
    ctx.r3.u32 = STATUS_SUCCESS;
}

void real_RtlNtStatusToDosError(PPCContext& ctx, uint8_t* base) {
    ctx.r3.u32 = 0; // ERROR_SUCCESS — mapeamento fino não é usado no boot
}

// ------------------------------------------------------------- printf

void real_DbgPrint(PPCContext& ctx, uint8_t* base) {
    ArgSource src{ctx, base};
    src.intBase = 4; // fmt em r3
    GuestArgs args = makeArgs(src);
    const std::string fmt = readGuestCString(ctx.r3.u64);
    char buf[1024];
    guestFormat(buf, sizeof(buf) - 1, fmt, args);
    __android_log_print(ANDROID_LOG_INFO, "FH2/DBG", "%s", buf);
    ctx.r3.u32 = 0;
}

void real_DbgBreakPoint(PPCContext& ctx, uint8_t* base) {
    RLOG("DbgBreakPoint (trap do guest — continuando)");
    ctx.r3.u32 = 0;
}

// _snprintf(dst, cnt, fmt, ...) — varargs a partir de r6
void real__snprintf(PPCContext& ctx, uint8_t* base) {
    ArgSource src{ctx, base};
    src.intBase = 6;
    GuestArgs args = makeArgs(src);
    const uint64_t dst = ctx.r3.u64;
    const uint64_t cnt = ctx.r4.u64;
    const std::string fmt = readGuestCString(ctx.r5.u64);
    if (!validGuest(dst, 1) || cnt == 0) {
        ctx.r3.u32 = 0xFFFFFFFFu;
        return;
    }
    char* buf = (char*)(base + dst);
    const int n = guestFormat(buf, std::min<uint64_t>(cnt, 0x8000), fmt, args);
    ctx.r3.u32 = (uint32_t)(int32_t)n;
}

// _vsnprintf(dst, cnt, fmt, va_list) — va_list guest = ponteiro p/ args
// salvos na stack do caller; v1: registra e trata como 0 args reais.
void real__vsnprintf(PPCContext& ctx, uint8_t* base) {
    const uint64_t dst = ctx.r3.u64;
    const uint64_t cnt = ctx.r4.u64;
    const std::string fmt = readGuestCString(ctx.r5.u64);
    if (!validGuest(dst, 1) || cnt == 0) {
        ctx.r3.u32 = 0xFFFFFFFFu;
        return;
    }
    // va_list do Xenon: ponteiro para área de 64 bytes (r3..r10 + f1..f8)
    const uint64_t va = ctx.r6.u64;
    ArgSource src{ctx, base};
    src.intBase = 4;
    GuestArgs args = makeArgs(src);
    if (validGuest(va, 64)) {
        // args salvos pelo caller: r4..r10 em [va+4..], lê como ints
        src.intCount = 0;
        // reconstrói nextInt lendo do va_list
        int i = 0;
        args.nextInt = [va, base, i = 0](int) mutable -> uint64_t {
            const uint64_t slot = va + 8ull * (i++) + 4; // u32 baixo do slot de 8
            return validGuest(slot, 4) ? g32(base, slot) : 0;
        };
    }
    char* buf = (char*)(base + dst);
    const int n = guestFormat(buf, std::min<uint64_t>(cnt, 0x8000), fmt, args);
    ctx.r3.u32 = (uint32_t)(int32_t)n;
}

void real_sprintf(PPCContext& ctx, uint8_t* base) {
    ArgSource src{ctx, base};
    src.intBase = 5; // dst=r3, fmt=r4, args=r5..
    GuestArgs args = makeArgs(src);
    const uint64_t dst = ctx.r3.u64;
    const std::string fmt = readGuestCString(ctx.r4.u64);
    if (!validGuest(dst, 1)) {
        ctx.r3.u32 = 0xFFFFFFFFu;
        return;
    }
    char* buf = (char*)(base + dst);
    const int n = guestFormat(buf, 0x8000, fmt, args);
    ctx.r3.u32 = (uint32_t)(int32_t)n;
}

void real_vsprintf(PPCContext& ctx, uint8_t* base) {
    const uint64_t dst = ctx.r3.u64;
    const std::string fmt = readGuestCString(ctx.r4.u64);
    const uint64_t va = ctx.r5.u64;
    if (!validGuest(dst, 1)) {
        ctx.r3.u32 = 0xFFFFFFFFu;
        return;
    }
    ArgSource src{ctx, base};
    src.intBase = 4;
    GuestArgs args = makeArgs(src);
    if (validGuest(va, 64)) {
        args.nextInt = [va, base, i = 0](int) mutable -> uint64_t {
            const uint64_t slot = va + 8ull * (i++) + 4; // u32 baixo do slot de 8
            return validGuest(slot, 4) ? g32(base, slot) : 0;
        };
    }
    char* buf = (char*)(base + dst);
    const int n = guestFormat(buf, 0x8000, fmt, args);
    ctx.r3.u32 = (uint32_t)(int32_t)n;
}

void real_vswprintf(PPCContext& ctx, uint8_t* base) {
    // (wchar* dst, size_t cnt, fmt, va_list)
    const uint64_t dst = ctx.r3.u64;
    const uint64_t cnt = ctx.r4.u64;
    const std::string fmt = readGuestCString(ctx.r5.u64);
    const uint64_t va = ctx.r6.u64;
    if (!validGuest(dst, 2) || cnt == 0) {
        ctx.r3.u32 = 0xFFFFFFFFu;
        return;
    }
    ArgSource src{ctx, base};
    src.intBase = 4;
    GuestArgs args = makeArgs(src);
    if (validGuest(va, 64)) {
        args.nextInt = [va, base, i = 0](int) mutable -> uint64_t {
            const uint64_t slot = va + 8ull * (i++) + 4; // u32 baixo do slot de 8
            return validGuest(slot, 4) ? g32(base, slot) : 0;
        };
    }
    const int n = guestFormatW((uint16_t*)(base + (uint32_t)dst),
                               std::min<uint64_t>(cnt, 0x4000), fmt, args);
    ctx.r3.u32 = (uint32_t)(int32_t)n;
}

// --------------------------------------------------------------- input

void real_XamInputGetState(PPCContext& ctx, uint8_t* base) {
    // (UserIndex, Flags, PXINPUT_STATE out)
    const uint64_t out = ctx.r5.u64;
    input::InputState* in = inputBridge();
    if (!in) {
        ctx.r3.u32 = 0x0000048F; // ERROR_DEVICE_NOT_CONNECTED
        return;
    }
    const int port = (int)ctx.r3.u32;
    if (port < 0 || port >= input::InputState::kMaxPorts) {
        ctx.r3.u32 = 0x0000048F;
        return;
    }
    const input::ControllerSnapshot s = in->snapshot(port);
    if (validGuest(out, 16)) {
        static std::atomic<uint32_t> packet{0};
        w32(base, out, packet.fetch_add(1) + 1); // dwPacketNumber
        const float tL = std::clamp(s.leftTrigger, 0.f, 1.f);
        const float tR = std::clamp(s.rightTrigger, 0.f, 1.f);
        const auto s16 = [](float v) {
            return (uint16_t)(int16_t)std::clamp((int)(v * 32767.f), -32768, 32767);
        };
        // XINPUT_GAMEPAD: wButtons, bL, bR, sLX, sLY, sRX, sRY (big-endian)
        const uint32_t buttons = s.buttons;
        const uint32_t trig = ((uint32_t)(tL * 255.f) << 24) |
                              ((uint32_t)(tR * 255.f) << 16);
        w32(base, out + 4, (buttons << 16) | (trig >> 16)); // wButtons+bL+bR
        w32(base, out + 8,
            ((uint32_t)s16(s.leftX) << 16) | (uint16_t)s16(s.leftY));
        w32(base, out + 12,
            ((uint32_t)s16(s.rightX) << 16) | (uint16_t)s16(s.rightY));
    }
    ctx.r3.u32 = 0; // ERROR_SUCCESS
}

void real_XamInputGetCapabilities(PPCContext& ctx, uint8_t* base) {
    // (UserIndex, Flags, PXINPUT_CAPABILITIES) — gamepad padrão conectado
    // XINPUT_CAPABILITIES: {Type u8, SubType u8, Flags u16, Gamepad(12), Vibration(8)}
    const uint64_t out = ctx.r5.u64;
    if (validGuest(out, 24)) {
        memset(base + out, 0, 24);
        base[out + 0] = 0x01; // XINPUT_DEVTYPE_GAMEPAD
        base[out + 1] = 0x01; // XINPUT_DEVSUBTYPE_GAMEPAD
        // wButtons suportados + triggers analógicos
        w32(base, out + 4, (0xF3FFu << 16) | 0xFF00u | 0x00FFu);
    }
    ctx.r3.u32 = 0;
}

void real_XamInputGetCapabilitiesEx(PPCContext& ctx, uint8_t* base) {
    real_XamInputGetCapabilities(ctx, base);
}

void real_XamInputSetState(PPCContext& ctx, uint8_t* base) {
    ctx.r3.u32 = 0; // vibração ignorada (sem hardware de vibração mapeado)
}

void real_XamGetCurrentTitleId(PPCContext& ctx, uint8_t* base) {
    ctx.r3.u32 = currentTitleId();
}

// Boot frio real: SEM launch data (nenhum convite/continue/dashboard args).
// (XamLoaderGetLaunchData/Size — implementações reais completas adiante,
//  junto de SetLaunchData/LaunchTitle/TerminateTitle)

void real_XamUserGetSigninState(PPCContext& ctx, uint8_t* base) {
    ctx.r3.u32 = 0; // sem perfil assinado (estado real v1: offline)
}

// ------------------------------------------------- módulos XEX
//
// No kernel real, o HANDLE de um módulo carregado é a base do header XEX
// (0x82000000 para o executável do título). XexGetModuleSection anda pelos
// section headers PE da imagem carregada e devolve (endereço, tamanho) da
// seção pedida — exatamente o que o CRT do título usa para localizar
// .pdata/.xdata/.tls e registrar exceptions/TLS.

inline uint32_t g16(uint8_t* b, uint64_t a) {
    return (uint32_t(b[a]) << 8) | b[a + 1];
}
// PE headers são LITTLE-ENDIAN (formato MS padrão) mesmo em imagens PPC
// big-endian — somente o CONTEÚDO das seções é big-endian. Confirmado no
// FH2: .rdata VA=0x400 size=0x36C030 em LE bate com Image::ParseImage.
inline uint32_t pe32(uint8_t* b, uint64_t a) {
    return (uint32_t)b[a] | ((uint32_t)b[a + 1] << 8) |
           ((uint32_t)b[a + 2] << 16) | ((uint32_t)b[a + 3] << 24);
}
inline uint32_t pe16(uint8_t* b, uint64_t a) {
    return (uint32_t)b[a] | ((uint32_t)b[a + 1] << 8);
}

// Localiza uma seção PE pelo nome (8 bytes, null-padded) na imagem guest.
static bool peFindSection(uint8_t* base, uint32_t hModule,
                          const std::string& name, uint32_t& outVa,
                          uint32_t& outSize) {
    if (!validGuest(hModule + 0x3C, 4)) return false;
    const uint32_t eLfanew = pe32(base, hModule + 0x3C);
    const uint32_t nt = hModule + eLfanew;
    if (!validGuest(nt, 24)) return false;
    if (pe32(base, nt) != 0x00004550u) return false; // 'PE\0\0' (LE)
    const uint32_t numSections = pe16(base, nt + 6);
    const uint32_t optSize = pe16(base, nt + 20);
    const uint32_t sections = nt + 4 + 20 + optSize;
    if (numSections > 96 || !validGuest(sections, size_t(numSections) * 40))
        return false;
    for (uint32_t i = 0; i < numSections; ++i) {
        const uint32_t sh = sections + i * 40;
        char secName[9];
        for (int c = 0; c < 8; ++c) secName[c] = (char)base[sh + c];
        secName[8] = 0;
        const size_t nl = name.size();
        const bool match = nl <= 8 && nl > 0 &&
                           memcmp(secName, name.c_str(), nl) == 0 &&
                           (nl == 8 || secName[nl] == 0);
        if (match) {
            outSize = pe32(base, sh + 8);   // VirtualSize (LE)
            outVa = pe32(base, sh + 12);    // VirtualAddress (LE)
            return true;
        }
    }
    return false;
}

void real_XexGetModuleHandle(PPCContext& ctx, uint8_t* base) {
    // (PCSZZ pszModuleName /*NULL = executável atual*/, PHANDLE phModule)
    // No console o handle de um módulo é a base do header XEX dele. Aqui:
    // executável → imageBase; "xam.xex"/"xboxkrnl.exe" → handle do módulo
    // virtual registrado no boot; nome desconhecido → executável (o título
    // nunca depende do handle além de opacidade + XexGetProcedureAddress).
    const uint64_t pName = ctx.r3.u64;
    uint64_t pOut = ctx.r4.u32;
    if (!validGuest(pOut, 4)) pOut = ctx.r3.u32; // variante de 1 argumento
    if (!validGuest(pOut, 4)) {
        ctx.r3.u32 = STATUS_INVALID_PARAMETER;
        return;
    }
    uint32_t handle = imageBase();
    if (pName != 0) {
        const std::string name = readGuestCString(pName, 64);
        if (!name.empty()) {
            const kern::KernelModule* m = kern::findModuleByName(name);
            if (m) handle = m->handle;
        }
    }
    w32(base, pOut, handle);
    ctx.r3.u32 = STATUS_SUCCESS;
    static Throttle t;
    if (t.shouldLog(4, 512)) {
        RLOG("XexGetModuleHandle(pName=0x%08llX) = 0x%08X",
             (unsigned long long)pName, handle);
    }
}

void real_XexGetModuleSection(PPCContext& ctx, uint8_t* base) {
    // (HANDLE hModule, LPCSTR pszName, PVOID* ppAddress, PDWORD pcbSize)
    // Semântica real: busca PRIMEIRO nos recursos nomeados do XEX
    // (XEX_HEADER_RESOURCE_INFO — "0D163575" no FH2), depois nas seções PE.
    const uint32_t hModule = ctx.r3.u32 ? ctx.r3.u32 : imageBase();
    const std::string name = readGuestCString(ctx.r4.u32, 8);
    const uint64_t ppAddr = ctx.r5.u32;
    const uint64_t pSize = ctx.r6.u32;
    if (name.empty() || !validGuest(ppAddr, 4) || !validGuest(pSize, 4)) {
        ctx.r3.u32 = STATUS_INVALID_PARAMETER;
        return;
    }
    // (1) recursos do XEX
    const KernelResource* res = findImageResource(name.c_str());
    if (res) {
        w32(base, ppAddr, res->va);
        w32(base, pSize, res->size);
        ctx.r3.u32 = STATUS_SUCCESS;
        RLOG("XexGetModuleSection('%s') = recurso 0x%08X (%u bytes)",
             name.c_str(), res->va, res->size);
        return;
    }
    // (2) seções PE da imagem
    uint32_t va = 0, size = 0;
    if (peFindSection(base, hModule, name, va, size)) {
        w32(base, ppAddr, hModule + va);
        w32(base, pSize, size);
        ctx.r3.u32 = STATUS_SUCCESS;
        RLOG("XexGetModuleSection('%s') = seção 0x%08X (%u bytes)",
             name.c_str(), hModule + va, size);
        return;
    }
    RLOG("XexGetModuleSection: '%s' não existe no módulo 0x%08X", name.c_str(),
         hModule);
    ctx.r3.u32 = STATUS_OBJECT_NAME_NOT_FOUND;
}

// ------------------------------------------------- XamLoader (loader XAM)

namespace {

// XAM launch data: buffer do kernel. O FH2 seta 1020 bytes de launch data no
// relaunch do launcher (medido no código recompilado: li r4,1020) — o clamp
// antigo de 512 CORROMPIA o mecanismo de relaunch (boot 2 nunca via os dados).
// Xenia não clampa (loader_data.launch_data.resize(size)); 4096 cobre o uso
// real do console com folga.
constexpr uint32_t kXamLaunchDataMax = 4096;

} // namespace

void real_XamLoaderSetLaunchData(PPCContext& ctx, uint8_t* base) {
    // (PVOID lpBuffer, DWORD cbSize) — guarda os dados de launch do título.
    const uint64_t data = ctx.r3.u64;
    const uint32_t size = ctx.r4.u32;
    if (size > kXamLaunchDataMax) {
        RLOG("XamLoaderSetLaunchData(data=%08llX, size=%u) = INVALID_PARAMETER "
             "(> %u)", (unsigned long long)data, size, kXamLaunchDataMax);
        ctx.r3.u32 = STATUS_INVALID_PARAMETER;
        return;
    }
    if (size && !validGuest(data, size)) {
        RLOG("XamLoaderSetLaunchData(data=%08llX, size=%u) = INVALID_PARAMETER "
             "(ponteiro guest inválido)", (unsigned long long)data, size);
        ctx.r3.u32 = STATUS_INVALID_PARAMETER;
        return;
    }
    setLaunchData(size ? base + data : nullptr, size);
    // Boot-critical: o relaunch do FH2 depende destes dados serem preservados
    // (o boot 2 lê GetLaunchDataSize/GetLaunchData e segue o boot real).
    RLOG("XamLoaderSetLaunchData(data=%08llX, size=%u) = SUCCESS — dados de "
         "launch preservados p/ o próximo boot",
         (unsigned long long)data, size);
    ctx.r3.u32 = 0; // ERROR_SUCCESS
}

void real_XamLoaderGetLaunchData(PPCContext& ctx, uint8_t* base) {
    // (PVOID lpBuffer, DWORD cbSize) — copia os dados de launch (se houver).
    const uint64_t buf = ctx.r3.u64;
    const uint32_t bufLen = ctx.r4.u32;
    uint8_t tmp[kXamLaunchDataMax];
    const uint32_t size = getLaunchData(tmp, sizeof(tmp));
    if (size == 0) {
        // Boot frio (sem launch data) — o título trata como boot inicial.
        RLOG("XamLoaderGetLaunchData(buf=%08llX, len=%u) = NOT_FOUND (boot "
             "frio)", (unsigned long long)buf, bufLen);
        ctx.r3.u32 = 0x80070490u; // ERROR_NOT_FOUND (boot frio — real)
        return;
    }
    if (!validGuest(buf, 4)) {
        ctx.r3.u32 = STATUS_INVALID_PARAMETER;
        return;
    }
    // copia min(size, bufLen) — como o kernel real
    const uint32_t copy = size < bufLen ? size : bufLen;
    if (copy) memcpy(base + buf, tmp, copy);
    RLOG("XamLoaderGetLaunchData(buf=%08llX, len=%u) = SUCCESS (%u bytes)",
         (unsigned long long)buf, bufLen, copy);
    ctx.r3.u32 = 0; // ERROR_SUCCESS
}

void real_XamLoaderGetLaunchDataSize(PPCContext& ctx, uint8_t* base) {
    // (PDWORD pcbSize) — tamanho dos dados de launch; sem dados: *pcb = 0 e
    // ERROR_NOT_FOUND (o título distingue boot frio de relaunch com dados).
    const uint32_t size = getLaunchDataSize();
    if (validGuest(ctx.r3.u64, 4)) w32(base, ctx.r3.u64, size);
    RLOG("XamLoaderGetLaunchDataSize(*size=%u) = %s", size,
         size ? "SUCCESS" : "NOT_FOUND");
    ctx.r3.u32 = size ? 0 : 0x80070490u; // ERROR_SUCCESS : ERROR_NOT_FOUND
}

void real_XamLoaderLaunchTitle(PPCContext& ctx, uint8_t* base) {
    // (LPCSTR pszPath, DWORD dwFlags) — NÃO RETORNA no console: o kernel
    // termina o título e o XAM o relança com o launch data preservado.
    // O runtime (PpcRuntime::run) faz o relaunch com estado zerado.
    const std::string path = readGuestCString(ctx.r3.u64, 512);
    const uint32_t flags = ctx.r4.u32;
    // Diagnóstico do relaunch: o launcher passa path NULL quando o padrão
    // interno casa (wrapper 0x82BF0870 faz li r3,0). Mostrar o estado real.
    RLOG("XamLoaderLaunchTitle(r3=%08llX \"%s\", flags=0x%08X) — encerrando "
         "título p/ relançar (não retorna)",
         (unsigned long long)ctx.r3.u64, path.c_str(), flags);
    requestTitleRelaunch(path, flags);
    jmp_buf* env = unwindTarget();
    if (env) longjmp(*env, 2); // unwind da thread chamadora — sem retorno
}

void real_XamLoaderTerminateTitle(PPCContext& ctx, uint8_t* base) {
    // () — NÃO RETORNA: encerra o título ativo.
    RLOG("XamLoaderTerminateTitle() — encerrando título (não retorna)");
    terminateTitle();
    jmp_buf* env = unwindTarget();
    if (env) longjmp(*env, 2);
}

// ------------------------------------------------- Xex* (módulos)

void real_XexLoadImage(PPCContext& ctx, uint8_t* base) {
    // (PCSZZ pszName, DWORD dwFlags, DWORD dwMinimumVersion, PHANDLE pHandle)
    // Módulos de SISTEMA (xam.xex, xboxkrnl.exe): sempre presentes no console
    // — aqui são módulos VIRTUAIS cujas exports são os stubs recompilados que
    // o título importa. Outros nomes: XEX secundário REAL (ex.
    // "game:\XMediaFacade_default.xex") — lido do storage do jogo, decodificado
    // (mesmo pipeline do loader) e mapeado na memória guest; as exports vêm
    // da tabela XEX_HEADER_EXPORTS_BY_NAME do módulo.
    const std::string name = readGuestCString(ctx.r3.u64, 128);
    const uint64_t pHandle = ctx.r6.u64;
    if (name.empty() || !validGuest(pHandle, 4)) {
        ctx.r3.u32 = STATUS_INVALID_PARAMETER;
        return;
    }
    if (const KernelModule* m = findModuleByName(name)) {
        w32(base, pHandle, m->handle);
        ctx.r3.u32 = STATUS_SUCCESS;
        static Throttle t;
        if (t.shouldLog(4, 64)) {
            RLOG("XexLoadImage(\"%s\") = 0x%08X (já carregado)", name.c_str(),
                 m->handle);
        }
        return;
    }
    // XEX secundário: caminho guest → relativo ao volume (game:\x → x,
    // update:\x → x — o launcher do FH2 também carrega pelo volume update)
    std::string rel = name;
    for (const char* pre : {"game:\\", "game:", "update:\\", "update:",
                            "D:\\", "d:\\"}) {
        const size_t n = strlen(pre);
        if (rel.size() > n && strncasecmp(rel.c_str(), pre, n) == 0) {
            rel = rel.substr(n);
            break;
        }
    }
    std::replace(rel.begin(), rel.end(), '\\', '/');
    uint32_t handle = 0;
    if (fsBridge()) {
        handle = fh2::ppc::loadSecondaryModule(*fsBridge(), rel, name);
    }
    if (handle == 0) {
        // honesto: arquivo ausente/inválido — mesmo status do console
        RLOG("XexLoadImage(\"%s\") = OBJECT_NAME_NOT_FOUND (arquivo ausente "
             "ou inválido no storage do jogo)", name.c_str());
        ctx.r3.u32 = 0xC0000034u; // STATUS_OBJECT_NAME_NOT_FOUND
        return;
    }
    w32(base, pHandle, handle);
    ctx.r3.u32 = STATUS_SUCCESS;
    RLOG("XexLoadImage(\"%s\", flags=0x%X) = 0x%08X (módulo carregado)",
         name.c_str(), ctx.r4.u32, handle);
}

void real_XexUnloadImage(PPCContext& ctx, uint8_t* base) {
    // (HANDLE hModule) — módulos de sistema permanecem carregados (real).
    const uint32_t h = ctx.r3.u32;
    if (!findModuleByHandle(h)) {
        ctx.r3.u32 = STATUS_INVALID_HANDLE;
        return;
    }
    ctx.r3.u32 = STATUS_SUCCESS;
}

void real_XexGetProcedureAddress(PPCContext& ctx, uint8_t* base) {
    // (HANDLE hModule, DWORD ordinal, PVOID* ppAddress) — devolve o VA do
    // stub do import (nop;nop;nop;blr) que a tabela mágica resolve para o
    // HLE — exatamente o endereço que o console devolveria p/ a função real.
    const uint32_t h = ctx.r3.u32;
    const uint32_t ordinal = ctx.r4.u32;
    const uint64_t pOut = ctx.r5.u64;
    if (!validGuest(pOut, 4)) {
        ctx.r3.u32 = STATUS_INVALID_PARAMETER;
        return;
    }
    const KernelModule* m = findModuleByHandle(h);
    if (!m) {
        ctx.r3.u32 = STATUS_INVALID_HANDLE;
        return;
    }
    auto it = m->exports.find(ordinal);
    if (it == m->exports.end()) {
        RLOG("XexGetProcedureAddress(0x%08X «%s», %u) = PROCEDURE_NOT_FOUND "
             "[guest lr=0x%08X]",
             h, m->name.c_str(), ordinal, (uint32_t)ctx.lr);
        ctx.r3.u32 = 0xC000007Au; // STATUS_PROCEDURE_NOT_FOUND
        return;
    }
    w32(base, pOut, it->second);
    ctx.r3.u32 = STATUS_SUCCESS;
    RLOG("XexGetProcedureAddress(0x%08X «%s», %u) = stub 0x%08X", h,
         m->name.c_str(), ordinal, it->second);
}

void real_XexCheckExecutablePrivilege(PPCContext& ctx, uint8_t* base) {
    // BOOL XexCheckExecutablePrivilege(DWORD privilege) — consulta REAL do
    // executive_flags do XEX carregado (0xFF00000100FF02FF no FH2).
    const uint32_t priv = ctx.r3.u32;
    ctx.r3.u32 = (imagePrivileges() & priv) ? 1 : 0;
}

// ------------------------------------------------- arquivos (Nt*)

// Converte um caminho de volume do 360 ("game:\x", "D:\x", "\??\D:\x") para
// o caminho RELATIVO dentro da pasta selecionada pelo usuário. O root da
// árvore SAF é o volume: "game:\media\foo.bar" → "media/foo.bar".
static std::string normalizeGuestPath(const std::string& in) {
    std::string p = in;
    // prefixos de namespace NT
    for (const char* pre : {"\\??\\", "\\Device\\"}) {
        const size_t n = strlen(pre);
        if (p.size() > n && strncasecmp(p.c_str(), pre, n) == 0) p = p.substr(n);
    }
    // volumes conhecidos: game:, d:, cdrom0 — ambos "game:\x" e "game:/x"
    // update: mapeia para a MESMA árvore (kernel real: sem title update
    // instalado o volume update cai no diretório do título — o FH2 lê
    // update:\media.zip da raiz do dump; mesmo mapeamento do Xenia).
    for (const char* vol : {"game:", "update:", "d:", "cdrom0:", "cache:"}) {
        const size_t n = strlen(vol);
        if (p.size() > n && strncasecmp(p.c_str(), vol, n) == 0 &&
            (p[n] == '\\' || p[n] == '/')) {
            p = p.substr(n + 1);
            break;
        }
    }
    for (char& c : p)
        if (c == '\\') c = '/';
    // componentes vazios fora
    std::string out;
    size_t start = 0;
    while (start < p.size()) {
        size_t end = p.find('/', start);
        std::string part = p.substr(start, end == std::string::npos ? std::string::npos : end - start);
        if (!part.empty() && part != ".") {
            if (part == "..") return {}; // traversal — caminho inválido
            if (!out.empty()) out += '/';
            out += part;
        }
        if (end == std::string::npos) break;
        start = end + 1;
    }
    return out;
}

// Lê STRING do guest — X_ANSI_STRING real do 360: {u16 Length; u16
// MaximumLength; u32 Buffer}, buffer ASCII (1 byte/char). O campo ObjectName
// de X_OBJECT_ATTRIBUTES é um PANSI_STRING (Xenia xboxkrnl_io.cc: o kernel
// traduz name_ptr como X_ANSI_STRING*), NÃO uma UNICODE_STRING UTF-16.
static std::string readAnsiString(uint8_t* base, uint64_t addr) {
    if (!validGuest(addr, 8)) return {};
    const uint32_t len = g16(base, addr);        // bytes
    const uint32_t buf = g32(base, addr + 4);
    if (len == 0 || len > 1024 || !validGuest(buf, len)) return {};
    std::string out;
    out.reserve(len);
    for (uint32_t i = 0; i < len; ++i) {
        const char c = (char)base[buf + i];
        if (c == 0) break;
        out.push_back(c);
    }
    return out;
}

// X_OBJECT_ATTRIBUTES REAL do Xbox 360 (Xenia src/xenia/xbox.h):
//   +0x00 u32 RootDirectory   (handle do objeto-pai p/ caminho relativo;
//                             0xFFFFFFFD = ObDosDevices — caminho DOS pleno)
//   +0x04 u32 NamePtr         (PANSI_STRING)
//   +0x0C u32 Attributes
// Retorna o caminho RELATIVO ao volume (vazio = não extraiu). O caminho
// bruto (p/ log) vai para *rawOut quando não-nulo.
static std::string pathFromObjectAttributes(uint8_t* base, uint64_t objAttr,
                                            std::string* rawOut = nullptr);
static bool resolveObSymbolicLink(std::string& raw);

static std::string pathFromObjectAttributes(uint8_t* base, uint64_t objAttr,
                                            std::string* rawOut) {
    if (rawOut) rawOut->clear();
    if (!validGuest(objAttr, 0x10)) return {};
    const uint32_t root = g32(base, objAttr);
    const uint32_t pName = g32(base, objAttr + 4);
    if (!pName) return {};
    std::string raw = readAnsiString(base, pName);
    if (rawOut) *rawOut = raw;
    // Symlinks do Object Manager (ObCreateSymbolicLink): resolve o nome
    // cheio ou o prefixo antes da normalização (semântica real do NT).
    resolveObSymbolicLink(raw);
    // ObDosDevices (0xFFFFFFFD) ou 0: o nome já é o caminho DOS completo
    // ("game:\media\video.xmv") — exatamente como o kernel real trata.
    if (root == 0 || root == 0xFFFFFFFDu) {
        return normalizeGuestPath(raw);
    }
    // Caminho relativo a um handle de arquivo/diretório já aberto: prefixa
    // com o caminho do objeto-raiz (semântica real do Object Manager).
    GuestFile* f = fileGet(root);
    if (!f) {
        RLOG("Nt*: root directory 0x%08X inválido (caminho '%s')", root,
             raw.c_str());
        return {};
    }
    std::string joined = f->path;
    if (!raw.empty() && raw[0] != '\\') {
        if (!joined.empty() && joined.back() != '/') joined += '/';
        joined += raw;
    } else {
        joined += raw; // "\x" relativo à raiz do objeto
    }
    return normalizeGuestPath(joined);
}

// Dispositivos de bloco do console — nomes NT canônicos (case-insensitive).
//   • \Device\Harddisk0\Partition0: o HD INTERNO do console — o título
//     formata/escreve a área de utilidade nele (XMountUtilityDrive);
//   • \Device\Cdrom0: o DISCO (imagem do usuário, leitura apenas).
static bool isHddPartition(const std::string& raw) {
    static const char kDev[] = R"(\Device\Harddisk0\Partition0)";
    const size_t n = strlen(kDev);
    return raw.size() >= n && strncasecmp(raw.c_str(), kDev, n) == 0;
}

static bool isCdromDevice(const std::string& raw) {
    static const char kDev[] = R"(\Device\Cdrom0)";
    const size_t n = strlen(kDev);
    return raw.size() >= n && strncasecmp(raw.c_str(), kDev, n) == 0;
}

// Tamanho do disco virtual (DVD5 1-camada: 2.299.540 setores de 2048 B).
constexpr uint64_t kRawDiscSize = 2299540ull * 2048ull;

// Partição do HD acessível ao título: 256 MB de backing file (sparse no
// host) cobrem com folga a área de utilidade (0xFF000) e o cache do título.
constexpr uint64_t kHddPartitionSize = 0x10000000ull;

// Geometria REAL da partição utilitária (cache) do console: 0xFF000 bytes,
// setores de 512 — os valores que o XMountUtilityDrive do XAM espera e
// valida (o jogo confere: setores>0, bytes/sector==512). Referência: kernel
// do Xenia (xboxkrnl_io.cc — X_IOCTL_DISK_GET_*).
static constexpr uint32_t kCachePartitionSize = 0xFF000u;
static constexpr uint32_t kCacheSectorSize = 512u;
static constexpr uint32_t k_IOCTL_DISK_GET_DRIVE_GEOMETRY = 0x00070000u;
static constexpr uint32_t k_IOCTL_DISK_GET_PARTITION_INFO = 0x00074004u;

// Partições de CACHE do HD interno (Cache0/Cache1): scratch gravável que o
// título usa p/ dados temporários. Sem HD (Arcade) elas NÃO existem no
// console — aqui mapeadas p/ storage local do app (real e persistente);
// filhos relativos resolvem em filesDir/cacheN/…
static bool isCachePartition(const std::string& raw) {
    const char* kCaches[] = {R"(\Device\Harddisk0\Cache0)",
                             R"(\Device\Harddisk0\Cache1)"};
    for (const char* dev : kCaches) {
        const size_t n = strlen(dev);
        if (strncasecmp(raw.c_str(), dev, n) == 0) return true;
    }
    return false;
}

// "\Device\Harddisk0\Cache0\sub\x" → "cache0/sub/x" (caminho local)
static std::string cachePartitionPath(const std::string& raw) {
    std::string p = raw;
    for (char& c : p)
        if (c == '\\') c = '/';
    const char* kPre[] = {"Device/Harddisk0/Cache0", "Device/Harddisk0/Cache1"};
    for (const char* pre : kPre) {
        const size_t n = strlen(pre);
        if (strncasecmp(p.c_str(), pre, n) == 0) {
            std::string vol = pre;
            vol = vol.substr(n - 6); // "CacheN"
            for (char& c : vol) c = (char)tolower((unsigned char)c);
            if (p.size() > n && p[n] == '/') return vol + "/" + p.substr(n + 1);
            return vol + "/";
        }
    }
    return {};
}

// Abre um handle de VOLUME-RAIZ/DIRETÓRIO (sem fd): os opens relativos
// (RootDirectory) ancoram no caminho deste objeto — semântica real do
// Object Manager. 'rel' vazio com display não-vazio = raiz de volume.
static bool trySpecialOpen(const std::string& raw, const std::string& rel,
                           bool write, uint32_t* outHandle,
                           std::string* outErr) {
    *outHandle = 0;
    GuestFile* f = nullptr;
    if (isHddPartition(raw)) {
        // \Device\Harddisk0\Partition0 = HD INTERNO do console (NÃO o
        // disco): o título formata e usa a área de utilidade nele
        // (XMountUtilityDrive — geometria 0xFF000 respondida no IOCTL).
        // Backing file REAL e persistente no storage do app.
        *outHandle = fileOpenBlockDevice(raw, "xbox_storage/hdd0.bin",
                                         kHddPartitionSize, &f);
        if (*outHandle) {
            RLOG("Nt*(HD interno) = 0x%08X (backing file persistente)",
                 *outHandle);
            return true;
        }
        *outErr = "hdd partition";
        return false;
    }
    if (isCdromDevice(raw)) {
        *outHandle = fileOpenRawDevice(raw, kRawDiscSize, &f);
        if (*outHandle) {
            RLOG("Nt*(raw disc device) = 0x%08X "
                 "(dispositivo de blocos virtual da mídia)",
                 *outHandle);
            return true;
        }
        *outErr = "raw device";
        return false;
    }
    if (isCachePartition(raw)) {
        // dispositivo BRUTO da partição (sem componente filho): block device
        // com backing persistente — o título formata estruturas nele
        static const char* kCacheDevs[] = {"\\Device\\Harddisk0\\Cache0",
                                           "\\Device\\Harddisk0\\Cache1"};
        for (int i = 0; i < 2; ++i) {
            const size_t n = strlen(kCacheDevs[i]);
            if (raw.size() == n && strncasecmp(raw.c_str(), kCacheDevs[i], n) == 0) {
                const std::string backing =
                    std::string("xbox_storage/cache") + char('0' + i) + ".img";
                *outHandle = fileOpenBlockDevice(raw, backing,
                                                 kCachePartitionSize, &f);
                if (*outHandle) {
                    RLOG("Nt*(cache%d device) = 0x%08X (backing persistente)",
                         i, *outHandle);
                    return true;
                }
                *outErr = "cache device";
                return false;
            }
        }
        const std::string local = cachePartitionPath(raw);
        // a raiz da partição vira handle de diretório com caminho local
        // ("cache0/…") — filhos relativos ancoram nele
        const bool isRoot = !local.empty() && local.back() == '/';
        if (!isRoot) {
            // ARQUIVO real sob a partição de cache (ex.: cache:\media.zip):
            // cria/lê no storage local do app — persistente entre relaunches
            GuestFile* g = nullptr;
            *outHandle = fileOpen(local, raw, write, &g);
            if (*outHandle) return true;
        }
        *outHandle = fileOpenDir(isRoot ? local.substr(0, local.size() - 1)
                                        : local, raw, &f);
        if (*outHandle) {
            f->dir = true;
            RLOG("Nt*(partição de cache) = 0x%08X (%s → storage local)",
                 *outHandle, raw.c_str());
            return true;
        }
        *outErr = "cache partition";
        return false;
    }
    if (rel.empty() && !raw.empty()) {
        // raiz de volume ("cache:\", "game:\") ou caminho de diretório
        *outHandle = fileOpenDir(rel, raw, &f);
        if (*outHandle) return true;
    }
    return false;
}

void real_NtCreateFile(PPCContext& ctx, uint8_t* base) {
    // (PHANDLE FileHandle, ACCESS_MASK, POBJECT_ATTRIBUTES, PIO_STATUS_BLOCK,
    //  PLARGE_INTEGER AllocationSize, ULONG FileAttributes, ULONG ShareAccess,
    //  ULONG CreateDisposition, ULONG CreateOptions)
    const uint64_t pHandle = ctx.r3.u32;
    const uint64_t objAttr = ctx.r5.u32;
    const uint64_t pIoStatus = ctx.r6.u32;
    const uint32_t disposition = ctx.r10.u32;
    if (!validGuest(pHandle, 4) || !validGuest(pIoStatus, 8)) {
        ctx.r3.u32 = STATUS_INVALID_PARAMETER;
        return;
    }
    std::string display;
    std::string rel = pathFromObjectAttributes(base, objAttr, &display);
    const bool write = disposition == 2 || disposition == 3 || disposition == 5;
    // FILE_SUPERSEDE=0, CREATE_NEW=1, CREATE_ALWAYS=2, OPEN_EXISTING=3,
    // OPEN_ALWAYS=4, TRUNCATE_EXISTING=5
    uint32_t handle = 0;
    {
        uint32_t h = 0;
        std::string err;
        if (trySpecialOpen(display, rel, write, &h, &err)) handle = h;
    }
    if (!handle && !rel.empty()) {
        GuestFile* f = nullptr;
        handle = fileOpen(rel, display, write, &f);
    }
    if (!handle) {
        w32(base, pIoStatus, STATUS_OBJECT_NAME_NOT_FOUND);
        ctx.r3.u32 = STATUS_OBJECT_NAME_NOT_FOUND;
        static Throttle tf;
        if (rel.empty()) {
            // caminho vazio: dumpa X_OBJECT_ATTRIBUTES p/ diagnóstico real
            RLOG("NtCreateFile('') = NOT_FOUND [guest lr=0x%08X objAttr=0x%08X "
                 "{root=0x%08X pName=0x%08X attr=0x%08X}]",
                 (uint32_t)ctx.lr, (uint32_t)objAttr,
                 validGuest(objAttr, 4) ? g32(base, objAttr) : 0,
                 validGuest(objAttr, 8) ? g32(base, objAttr + 4) : 0,
                 validGuest(objAttr, 16) ? g32(base, objAttr + 12) : 0);
        } else if (tf.shouldLog(24, 512)) {
            RLOG("NtCreateFile('%s') = NOT_FOUND [guest lr=0x%08X]",
                 display.c_str(), (uint32_t)ctx.lr);
        }
        return;
    }
    w32(base, pHandle, handle);
    w32(base, pIoStatus, 0);            // Status = SUCCESS
    w32(base, pIoStatus + 4, 0);        // Information = 0
    ctx.r3.u32 = STATUS_SUCCESS;
    static Throttle t;
    if (t.shouldLog(24, 512)) {
        GuestFile* f = fileGet(handle);
        RLOG("NtCreateFile('%s') = 0x%08X (%llu bytes%s)", display.c_str(),
             handle, (unsigned long long)(f ? f->size : 0),
             write ? ", write" : "");
    }
}

void real_NtOpenFile(PPCContext& ctx, uint8_t* base) {
    // (PHANDLE, ACCESS_MASK, POBJECT_ATTRIBUTES, PIO_STATUS_BLOCK,
    //  ULONG ShareAccess, ULONG OpenOptions)
    const uint64_t pHandle = ctx.r3.u32;
    const uint64_t objAttr = ctx.r5.u32;
    const uint64_t pIoStatus = ctx.r6.u32;
    if (!validGuest(pHandle, 4) || !validGuest(pIoStatus, 8)) {
        ctx.r3.u32 = STATUS_INVALID_PARAMETER;
        return;
    }
    std::string display;
    std::string rel = pathFromObjectAttributes(base, objAttr, &display);
    uint32_t handle = 0;
    {
        uint32_t h = 0;
        std::string err;
        if (trySpecialOpen(display, rel, write, &h, &err)) handle = h;
    }
    if (!handle && !rel.empty()) {
        GuestFile* f = nullptr;
        handle = fileOpen(rel, display, false, &f);
    }
    if (!handle) {
        w32(base, pIoStatus, STATUS_OBJECT_NAME_NOT_FOUND);
        ctx.r3.u32 = STATUS_OBJECT_NAME_NOT_FOUND;
        static Throttle tf;
        if (rel.empty()) {
            RLOG("NtOpenFile('') = NOT_FOUND [guest lr=0x%08X objAttr=0x%08X "
                 "{root=0x%08X pName=0x%08X attr=0x%08X}]",
                 (uint32_t)ctx.lr, (uint32_t)objAttr,
                 validGuest(objAttr, 4) ? g32(base, objAttr) : 0,
                 validGuest(objAttr, 8) ? g32(base, objAttr + 4) : 0,
                 validGuest(objAttr, 16) ? g32(base, objAttr + 12) : 0);
        } else if (tf.shouldLog(24, 512)) {
            RLOG("NtOpenFile('%s') = NOT_FOUND [guest lr=0x%08X]",
                 display.c_str(), (uint32_t)ctx.lr);
        }
        return;
    }
    w32(base, pHandle, handle);
    w32(base, pIoStatus, 0);
    w32(base, pIoStatus + 4, 0);
    ctx.r3.u32 = STATUS_SUCCESS;
    static Throttle t;
    if (t.shouldLog(24, 512)) {
        GuestFile* f = fileGet(handle);
        RLOG("NtOpenFile('%s') = 0x%08X (%llu bytes)", display.c_str(),
             handle, (unsigned long long)(f ? f->size : 0));
    }
}

void real_NtReadFile(PPCContext& ctx, uint8_t* base) {
    // (HANDLE, HANDLE Event, PIO_APC_ROUTINE, PVOID ApcContext,
    //  PIO_STATUS_BLOCK, PVOID Buffer, ULONG Length,
    //  PLARGE_INTEGER ByteOffset, PULONG Key)
    const uint32_t handle = ctx.r3.u32;
    const uint32_t event = ctx.r4.u32;
    const uint64_t pIoStatus = ctx.r7.u32;
    const uint64_t buffer = ctx.r8.u32;
    const uint32_t length = ctx.r9.u32;
    const uint64_t pOffset = ctx.r10.u32;
    GuestFile* f = fileGet(handle);
    if (!f || !validGuest(pIoStatus, 8)) {
        ctx.r3.u32 = STATUS_INVALID_HANDLE;
        return;
    }
    if (length > 0 && !validGuest(buffer, 1)) {
        ctx.r3.u32 = STATUS_ACCESS_VIOLATION;
        return;
    }
    uint64_t off;
    if (pOffset && validGuest(pOffset, 8)) {
        off = g64(base, pOffset); // PLARGE_INTEGER (u64 BE)
    } else {
        off = f->pos;             // ponteiro lógico do FILE_OBJECT
    }

    // ------------------------------------------------ dispositivo de blocos
    // \Device\Harddisk0\Partition0: setores de 2048 B.
    //   • MODO ISO: fd real da imagem — TODOS os bytes são os do disco do
    //     usuário (partição GDFX completa, incluindo setor 32 real).
    //   • MODO PASTA: mídia virtual — setor 32 sintético (magic XDVDFS) e
    //     zeros no restante (área não presente num dump de arquivos). Cada
    //     leitura é logada: o padrão do título mostra o que falta servir.
    if (f->rawDevice) {
        // MODO ISO / BACKING FILE: fd real — bytes reais do disco ou da
        // partição persistente via pread. Regiões nunca escritas do backing
        // (setores além do EOF) leem ZEROS — semântica de disco real.
        if (f->fd >= 0) {
            uint32_t done = 0;
            if (length > 0 && off < f->size) {
                const uint64_t want = std::min<uint64_t>(length, f->size - off);
                ssize_t n;
                do {
                    n = pread(f->fd, base + buffer + done, want - done,
                              (off_t)(off + done));
                } while (n > 0 && (done += (uint32_t)n) < want);
                done = (uint32_t)std::min<uint64_t>(done, want);
                if (done < want) memset(base + buffer + done, 0, want - done);
                f->pos = off + done;
            }
            w32(base, pIoStatus, 0);
            w32(base, pIoStatus + 4, done);
            ctx.r3.u32 = STATUS_SUCCESS;
            static Throttle t;
            if (t.shouldLog(8, 256)) {
                RLOG("NtReadFile(Partition0/ISO) setor=%llu len=%u = %u bytes",
                     (unsigned long long)(off / 2048), length, done);
            }
            return;
        }
        uint32_t done = 0;
        if (length > 0 && off < f->size) {
            done = (uint32_t)std::min<uint64_t>(length, f->size - off);
            memset(base + buffer, 0, done);
            constexpr uint64_t kVdOff = 32ull * 2048ull; // setor 32 = XDVDFS
            if (off < kVdOff + 2048 && off + done > kVdOff) {
                uint8_t vd[2048];
                memset(vd, 0, sizeof(vd));
                memcpy(vd, "MICROSOFT*XBOX*MEDIA", 20);
                // root_dir_sector/size/checksum: 0 — a montagem real da
                // árvore (issue #19 GDFX) substituirá os valores; o padrão
                // de leitura do título define o próximo passo REAL.
                const uint64_t dst = off > kVdOff ? 0 : kVdOff - off;
                const uint64_t src = off > kVdOff ? off - kVdOff : 0;
                const uint64_t n =
                    std::min<uint64_t>(2048 - src, done - dst);
                memcpy(base + buffer + dst, vd + src, n);
            }
            f->pos = off + done;
        }
        w32(base, pIoStatus, 0);
        w32(base, pIoStatus + 4, done);
        ctx.r3.u32 = STATUS_SUCCESS;
        static Throttle t;
        if (t.shouldLog(8, 256)) {
            RLOG("NtReadFile(Partition0) setor=%llu len=%u = %u bytes",
                 (unsigned long long)(off / 2048), length, done);
        }
        return;
    }
    if (f->dir) {
        // leitura em handle de diretório: erro real do gerenciador de E/S
        w32(base, pIoStatus, 0xC000000Bu); // STATUS_INVALID_HANDLE class
        ctx.r3.u32 = 0xC000000Bu;
        return;
    }
    if (f->fd < 0) {
        ctx.r3.u32 = STATUS_INVALID_HANDLE;
        return;
    }
    uint32_t done = 0;
    if (length > 0) {
        if (off >= f->size) {
            done = 0; // EOF real
        } else {
            const uint64_t want = std::min<uint64_t>(length, f->size - off);
            ssize_t n;
            do {
                n = pread(f->fd, base + buffer + done, want - done,
                          (off_t)(f->baseOffset + off + done));
            } while (n > 0 && (done += (uint32_t)n) < want);
            done = (uint32_t)std::min<uint64_t>(done, want);
        }
        f->pos = off + done;
    }
    w32(base, pIoStatus, 0);       // Status
    w32(base, pIoStatus + 4, done); // Information = bytes lidos
    ctx.r3.u32 = STATUS_SUCCESS;
    // sinaliza evento de conclusão (I/O síncrono com evento opcional)
    if (event) {
        Waitable* w = waitableByHandle(event);
        if (w) setEvent(w);
    }
    static Throttle t;
    if (t.shouldLog(32, 1024)) {
        RLOG("NtReadFile('%s' off=%llu len=%u) = %u bytes", f->display.c_str(),
             (unsigned long long)off, length, done);
    }
}

void real_NtQueryInformationFile(PPCContext& ctx, uint8_t* base) {
    // (HANDLE, PIO_STATUS_BLOCK, PVOID FileInformation, ULONG Length,
    //  FILE_INFORMATION_CLASS)
    const uint32_t handle = ctx.r3.u32;
    const uint64_t pIoStatus = ctx.r4.u32;
    const uint64_t info = ctx.r5.u32;
    const uint32_t length = ctx.r6.u32;
    const uint32_t cls = ctx.r7.u32;
    GuestFile* f = fileGet(handle);
    if (!f || !validGuest(pIoStatus, 8)) {
        ctx.r3.u32 = STATUS_INVALID_HANDLE;
        return;
    }
    // FileStandardInformation = 5: {AllocSize u64, EndOfFile u64,
    //  NumberOfLinks u32, DeletePending u8, Directory u8}
    if (cls == 5) {
        if (length >= 24 && validGuest(info, 24)) {
            w64(base, info, f->size);       // AllocationSize
            w64(base, info + 8, f->size);   // EndOfFile
            w32(base, info + 16, 1);        // NumberOfLinks
            base[info + 20] = 0;            // DeletePending
            base[info + 21] = 0;            // Directory = false
        }
    } else if (cls == 22) { // FileNetworkOpenInformation = 22 (56 bytes)
        if (length >= 56 && validGuest(info, 56)) {
            const uint64_t ft = 0; // timestamps reais ficam p/ XContent
            w64(base, info + 32, f->size);  // EndOfFile
            w64(base, info + 40, f->size);  // AllocationSize
            (void)ft;
        }
    }
    w32(base, pIoStatus, 0);
    w32(base, pIoStatus + 4, 0);
    ctx.r3.u32 = STATUS_SUCCESS;
}

void real_NtQueryVolumeInformationFile(PPCContext& ctx, uint8_t* base) {
    // (HANDLE, PIO_STATUS_BLOCK, PVOID FsInformation, ULONG Length, CLASS)
    const uint64_t pIoStatus = ctx.r4.u32;
    const uint64_t info = ctx.r5.u32;
    const uint32_t length = ctx.r6.u32;
    const uint32_t cls = ctx.r7.u32;
    if (!validGuest(pIoStatus, 8)) {
        ctx.r3.u32 = STATUS_INVALID_HANDLE;
        return;
    }
    // FileFsSizeInformation = 3: {TotalAllocation u64, Available u64,
    //  SectorsPerCluster u32, BytesPerSector u32} — geometria real de DVD.
    if (cls == 3 && length >= 32 && validGuest(info, 32)) {
        w64(base, info, (uint64_t)8589934592ull);      // 8 GB de alocação
        w64(base, info + 8, (uint64_t)4294967296ull);  // disponível
        w32(base, info + 16, 16);   // sectores por cluster
        w32(base, info + 20, 2048); // bytes por sector (DVD)
    } else if (cls == 4 && length >= 24 && validGuest(info, 24)) {
        // FileFsVolumeInformation: {VolumeCreationTime u64, SerialNumber u32,
        //  VolumeLabelLength u32, SupportsObjects u8, Label...}
        w64(base, info, 0);
        w32(base, info + 8, 0x12345678); // serial
        w32(base, info + 12, 7);
        base[info + 16] = 1;
        memcpy(base + info + 17, "FH2DVD", 6);
    }
    w32(base, pIoStatus, 0);
    w32(base, pIoStatus + 4, 0);
    ctx.r3.u32 = STATUS_SUCCESS;
}

void real_NtSetInformationFile(PPCContext& ctx, uint8_t* base) {
    // (HANDLE, PIO_STATUS_BLOCK, PVOID, ULONG, CLASS) — v1: aceita e ignora
    // (o jogo usa p/ truncar files de save; posição EOF gravada no close).
    const uint64_t pIoStatus = ctx.r4.u32;
    if (validGuest(pIoStatus, 8)) {
        w32(base, pIoStatus, 0);
        w32(base, pIoStatus + 4, 0);
    }
    ctx.r3.u32 = STATUS_SUCCESS;
}

void real_NtFlushBuffersFile(PPCContext& ctx, uint8_t* base) {
    const uint64_t pIoStatus = ctx.r4.u32;
    if (validGuest(pIoStatus, 8)) {
        w32(base, pIoStatus, 0);
        w32(base, pIoStatus + 4, 0);
    }
    ctx.r3.u32 = STATUS_SUCCESS;
}

void real_NtQueryFullAttributesFile(PPCContext& ctx, uint8_t* base) {
    // (POBJECT_ATTRIBUTES, PFILE_NETWORK_OPEN_INFORMATION)
    const uint64_t objAttr = ctx.r3.u32;
    const uint64_t info = ctx.r4.u32;
    std::string display;
    const std::string rel = pathFromObjectAttributes(base, objAttr, &display);
    if (rel.empty() || !validGuest(info, 56)) {
        ctx.r3.u32 = STATUS_OBJECT_NAME_NOT_FOUND;
        return;
    }
    // sondagem real: abre e fecha (sem handle visível)
    GuestFile* probe = nullptr;
    const uint32_t h = fileOpen(rel, display, false, &probe);
    if (!h) {
        ctx.r3.u32 = STATUS_OBJECT_NAME_NOT_FOUND;
        return;
    }
    const uint64_t size = probe->size;
    fileClose(h);
    memset(base + info, 0, 56);
    w64(base, info + 32, size);  // EndOfFile
    w64(base, info + 40, size);  // AllocationSize
    ctx.r3.u32 = STATUS_SUCCESS;
}

void real_NtWriteFile(PPCContext& ctx, uint8_t* base) {
    // (HANDLE, HANDLE Event, PIO_APC_ROUTINE, PVOID ApcContext,
    //  PIO_STATUS_BLOCK, PVOID Buffer, ULONG Length,
    //  PLARGE_INTEGER ByteOffset, PULONG Key)
    const uint32_t handle = ctx.r3.u32;
    const uint32_t event = ctx.r4.u32;
    const uint64_t pIoStatus = ctx.r7.u32;
    const uint64_t buffer = ctx.r8.u32;
    const uint32_t length = ctx.r9.u32;
    const uint64_t pOffset = ctx.r10.u32;
    GuestFile* f = fileGet(handle);
    if (!f || f->fd < 0 || !validGuest(pIoStatus, 8)) {
        ctx.r3.u32 = STATUS_INVALID_HANDLE;
        return;
    }
    if (!f->write) {
        // volume do jogo é read-only no kernel real (disco)
        ctx.r3.u32 = STATUS_ACCESS_DENIED;
        return;
    }
    if (length > 0 && !validGuest(buffer, 1)) {
        ctx.r3.u32 = STATUS_ACCESS_VIOLATION;
        return;
    }
    uint64_t off;
    if (pOffset && validGuest(pOffset, 8)) {
        off = g64(base, pOffset);
    } else {
        off = f->pos;
    }
    uint32_t done = 0;
    if (length > 0) {
        ssize_t n;
        do {
            n = pwrite(f->fd, base + buffer + done, length - done,
                       (off_t)(off + done));
        } while (n > 0 && (done += (uint32_t)n) < length);
        if (n < 0 && done == 0) {
            // ENOSPC/EIO: o disco do host está cheio — condição REAL que o
            // título trata (o console a reporta igualmente)
            w32(base, pIoStatus, 0xC000007Fu); // STATUS_DISK_FULL
            ctx.r3.u32 = 0xC000007Fu;
            return;
        }
        if (off + done > f->size) f->size = off + done;
        f->pos = off + done;
    }
    w32(base, pIoStatus, 0);
    w32(base, pIoStatus + 4, done);
    ctx.r3.u32 = STATUS_SUCCESS;
    if (event) {
        Waitable* w = waitableByHandle(event);
        if (w) setEvent(w);
    }
}

void real_NtReadFileScatter(PPCContext& ctx, uint8_t* base) {
    // Semântica real: o scatter do 360 lê páginas em segmentos — para o
    // kernel HLE, o caminho de dados é o mesmo do NtReadFile.
    real_NtReadFile(ctx, base);
}

// ------------------------------------------------------------- rastreio

namespace {

constexpr size_t kTraceRingSize = 256;   // entradas do ring buffer
constexpr uint64_t kTraceFullLogLimit = 3000; // primeiras N linhas no log
constexpr uint64_t kTraceSampleEvery = 512;   // depois, 1 a cada N

struct TraceEntry {
    uint64_t seq;
    char text[152];
};

struct TraceState {
    std::mutex m;
    TraceEntry ring[kTraceRingSize];
    uint64_t next = 0;      // índice de escrita circular
    uint64_t count = 0;     // total gravado
    uint64_t seq = 0;
};

TraceState& traceState() {
    static TraceState s;
    return s;
}

thread_local uint64_t t_traceArgs[6] = {0, 0, 0, 0, 0, 0};
thread_local bool t_traceHasArgs = false;

} // namespace

void traceCall(const char* name, const PPCContext& ctx) {
    t_traceArgs[0] = ctx.r3.u64;
    t_traceArgs[1] = ctx.r4.u64;
    t_traceArgs[2] = ctx.r5.u64;
    t_traceArgs[3] = ctx.r6.u64;
    t_traceArgs[4] = ctx.r7.u64;
    t_traceArgs[5] = ctx.r8.u64;
    t_traceHasArgs = true;
    (void)name;
}

void traceReturn(const char* name, const PPCContext& ctx) {
    uint64_t a[6];
    if (t_traceHasArgs) {
        for (int i = 0; i < 6; ++i) a[i] = t_traceArgs[i];
        t_traceHasArgs = false;
    } else {
        a[0] = ctx.r3.u64; // chamada sem traceCall (não deve ocorrer)
        for (int i = 1; i < 6; ++i) a[i] = 0;
    }
    const uint64_t ret = ctx.r3.u64;

    // Filtro do ring: Rtl{Enter,Leave}CriticalSection dominam o traço (a
    // thread principal gira nelas enquanto o launcher trabalha) e escondem a
    // cauda útil. Seguem no log vivo (throttle); não entram no ring/dump.
    if (name && (strcmp(name, "RtlEnterCriticalSection") == 0 ||
                 strcmp(name, "RtlLeaveCriticalSection") == 0)) {
        return;
    }

    TraceState& s = traceState();
    TraceEntry e;
    e.seq = 0;
    snprintf(e.text, sizeof(e.text),
             "%-34s r3=%08llX r4=%08llX r5=%08llX -> %08llX",
             name, (unsigned long long)a[0], (unsigned long long)a[1],
             (unsigned long long)a[2], (unsigned long long)ret);
    {
        std::lock_guard<std::mutex> lk(s.m);
        e.seq = s.seq;
        s.ring[s.next % kTraceRingSize] = e;
        ++s.next;
        ++s.seq;
        s.count = s.next;
    }
    const uint64_t n = e.seq + 1;
    if (n <= kTraceFullLogLimit || (n % kTraceSampleEvery) == 0) {
        RLOG("[TRACE %6llu] %s", (unsigned long long)n, e.text);
    }
}

void traceDump(const char* reason) {
    TraceState& s = traceState();
    __android_log_print(ANDROID_LOG_WARN, "FH2/TRACE",
                        "=== DUMP (%s) — últimas chamadas HLE ===", reason);
    std::lock_guard<std::mutex> lk(s.m);
    const uint64_t total = s.count;
    const uint64_t start = total > kTraceRingSize ? total - kTraceRingSize : 0;
    for (uint64_t i = start; i < total; ++i) {
        const TraceEntry& e = s.ring[i % kTraceRingSize];
        __android_log_print(ANDROID_LOG_WARN, "FH2/TRACE",
                            "[%6llu] %s", (unsigned long long)(e.seq + 1),
                            e.text);
    }
    __android_log_print(ANDROID_LOG_WARN, "FH2/TRACE",
                        "=== FIM DO DUMP (%s) — %llu chamadas registradas ===",
                        reason, (unsigned long long)total);
}

// ================================================= Vd* — vídeo/swap real
//
// Semântica REAL do xboxkrnl para o caminho de vídeo do título:
//   • toda configuração que o jogo entrega (endereços do GPU identifier,
//     ring buffer, callbacks, modos) é ARMAZENADA de verdade em
//     vdGraphics() — é exatamente o que o kernel do 360 faz;
//   • VdSwap executa o present real no backend ativo (Vulkan/GLES): acquire,
//     render, submit e flip bloqueante = vblank do display Android;
//   • VdGetSystemCommandBuffer aloca um buffer de sistema REAL na janela
//     virtual do guest (o guest escreve nele; o command processor da
//     issue #17 o consome);
//   • a chamada do graphics interrupt (callback registrado) exige thread de
//     interrupção com TEB próprio — os endereços ficam guardados para o
//     marco do processador de comandos Xenos (issue #17).
VdGraphicsState& vdGraphics() {
    static VdGraphicsState s;
    return s;
}

void real_VdSwap(PPCContext& ctx, uint8_t* base) {
    // Semântica REAL do console (mesma do Xenia — xboxkrnl_video.cc VdSwap):
    //   VdSwap(buffer_ptr, fetch_ptr, unk2, unk3, unk4, frontbuffer_ptr,
    //          texture_format_ptr, color_space_ptr, width_ptr, height_ptr)
    // O D3D9 do título reserva 64 dwords no ring primário e passa:
    //   • fetch_ptr: texture fetch do front buffer (6 dwords, xe_gpu_texture_fetch_t)
    //   • frontbuffer_ptr/format/width/height: ponteiros p/ valores reais
    // O kernel TRADUZ o VA do fetch para PA, grava no buffer_ptr um pacote
    // type-0 (fetch → SHADER_CONSTANT_FETCH_00_0) + PM4_XE_SWAP + NOPs; o CP
    // (cmd_processor.cpp) processa o ring e apresenta o frame REAL.
    auto& vd = vdGraphics();
    vd.swapCount += 1;

    const uint32_t bufferPtr = uint32_t(ctx.r3.u64);
    const uint32_t fetchPtr = uint32_t(ctx.r4.u64);
    // Ordem REAL dos parâmetros (xenia xboxkrnl_video.cc VdSwap_entry):
    //   r3 buffer_ptr, r4 fetch_ptr, r5 unk2, r6 unk3, r7 unk4,
    //   r8 frontbuffer_ptr, r9 texture_format_ptr, r10 color_space_ptr,
    //   r11 width, r12 height
    const uint32_t frontbufferPtr = uint32_t(ctx.r8.u64);
    const uint32_t fmtPtr = uint32_t(ctx.r9.u64);
    const uint32_t colorSpacePtr = uint32_t(ctx.r10.u64);
    const uint32_t widthPtr = uint32_t(ctx.r11.u64);
    const uint32_t heightPtr = uint32_t(ctx.r12.u64);
    (void)colorSpacePtr; // RGB (0) no FH2 — verificado pelo guest

    if (!validGuest(bufferPtr, 64 * 4) || !validGuest(fetchPtr, 24) ||
        !validGuest(frontbufferPtr, 4) || !validGuest(fmtPtr, 4) ||
        !validGuest(widthPtr, 4) || !validGuest(heightPtr, 4)) {
        RLOG("VdSwap: argumentos inválidos (buffer=0x%08X fetch=0x%08X) — "
             "swap descartado",
             bufferPtr, fetchPtr);
        ctx.r3.u32 = STATUS_INVALID_PARAMETER;
        return;
    }

    // Texture fetch REAL (6 dwords BE do guest).
    uint32_t fetch[6];
    for (int i = 0; i < 6; ++i) fetch[i] = g32(base, fetchPtr + i * 4);
    // dword_1: format : 6 | endian : 2 | request : 2 | stacked : 1 | ncp : 1
    //          | base_address : 20 (VA >> 12)
    const uint32_t frontBufferVa = (fetch[1] >> 12) << 12;
    // Tradução VA→PA REAL do modelo de memória do console.
    uint32_t frontBufferPa;
    if (frontBufferVa >= 0x80000000u && frontBufferVa < 0xA0000000u) {
        frontBufferPa = frontBufferVa - 0x80000000u;
    } else if (frontBufferVa >= 0xA0000000u && frontBufferVa < 0xC0000000u) {
        frontBufferPa = frontBufferVa - 0xA0000000u;
    } else {
        RLOG("VdSwap: front buffer VA inválido 0x%08X — swap descartado",
             frontBufferVa);
        ctx.r3.u32 = STATUS_INVALID_PARAMETER;
        return;
    }
    fetch[1] = (fetch[1] & 0xFFFu) | ((frontBufferPa >> 12) << 12);

    // Verificação REAL: o frontbuffer_ptr do D3D deve apontar o mesmo VA.
    const uint32_t d3dFrontVa = g32(base, frontbufferPtr);
    if (d3dFrontVa != frontBufferVa) {
        static Throttle tMismatch;
        if (tMismatch.shouldLog(4, 1024)) {
            RLOG("VdSwap: front buffer do fetch (0x%08X) != do D3D (0x%08X) — "
                 "usando o do fetch", frontBufferVa, d3dFrontVa);
        }
    }

    // Enfileira os 64 dwords no ring (BE — memória guest):
    //   type-0 (0x4800 FETCH_00_0, 6 dwords) + type-3 XE_SWAP (4) + NOPs
    const uint32_t type0 = (0u << 30) | ((6u - 1u) << 16) | 0x4800;
    const uint32_t type3Swap =
        (3u << 30) | ((4u - 1u) << 16) | (0x64 << 8); // PM4_XE_SWAP
    const uint32_t type2Nop = 2u << 30;
    uint32_t off = 0;
    w32(base, bufferPtr + off * 4, type0);
    ++off;
    for (int i = 0; i < 6; ++i, ++off) w32(base, bufferPtr + off * 4, fetch[i]);
    w32(base, bufferPtr + off * 4, type3Swap);
    ++off;
    w32(base, bufferPtr + off * 4, 0x53574150u); // fourcc "SWAP"
    ++off;
    w32(base, bufferPtr + off * 4, frontBufferPa);
    ++off;
    w32(base, bufferPtr + off * 4, g32(base, widthPtr));
    ++off;
    w32(base, bufferPtr + off * 4, g32(base, heightPtr));
    ++off;
    for (; off < 64; ++off) w32(base, bufferPtr + off * 4, type2Nop);

    static Throttle t1;
    if (t1.shouldLog(8, 512)) {
        RLOG("VdSwap: pacote enfileirado #%llu (front PA=0x%08X %ux%u fmt=%u) "
             "— CP apresenta",
             (unsigned long long)vd.swapCount, frontBufferPa,
             g32(base, widthPtr), g32(base, heightPtr), g32(base, fmtPtr));
    }
    ctx.r3.u32 = STATUS_SUCCESS;
}

void real_VdQueryVideoMode(PPCContext& ctx, uint8_t* base) {
    // X_VIDEO_MODE real do 360: 1280x720@60 progressivo hi-def.
    if (validGuest(ctx.r3.u64, 44)) {
        w32(base, ctx.r3.u64 + 0x00, 1280);   // displayWidth
        w32(base, ctx.r3.u64 + 0x04, 720);    // displayHeight
        w32(base, ctx.r3.u64 + 0x08, 0);      // interlaced = FALSE
        w32(base, ctx.r3.u64 + 0x0C, 60);     // refreshRate
        w32(base, ctx.r3.u64 + 0x10, 1);      // videoStandard = NTSC_M
        w32(base, ctx.r3.u64 + 0x14, 0);      // flags
        w32(base, ctx.r3.u64 + 0x18, 1);      // displayMode
        w32(base, ctx.r3.u64 + 0x1C, 1);      // isHiDef = TRUE
        w32(base, ctx.r3.u64 + 0x20, 0);      // reserved[0..2]
        w32(base, ctx.r3.u64 + 0x24, 0);
        w32(base, ctx.r3.u64 + 0x28, 0);
    }
    ctx.r3.u32 = STATUS_SUCCESS;
}

void real_VdQueryVideoFlags(PPCContext& ctx, uint8_t* base) {
    (void)base;
    // Sem flags especiais de AV pack (480p/widescreen via QueryVideoMode).
    ctx.r3.u32 = 0;
}

void real_VdGetCurrentDisplayGamma(PPCContext& ctx, uint8_t* base) {
    (void)base;
    // Gamma padrão do console: 2.2 (valor 1 da enumeração do kernel).
    ctx.r3.u32 = 1;
}

void real_VdGetCurrentDisplayInformation(PPCContext& ctx, uint8_t* base) {
    // X_DISPLAY_INFORMATION (0x80 bytes): sem EDID do monitor virtual —
    // zeros reais (o console bootado em TV desconhecida reporta o mesmo).
    if (validGuest(ctx.r3.u64, 0x80)) {
        memset(base + ctx.r3.u64, 0, 0x80);
    }
    ctx.r3.u32 = STATUS_SUCCESS;
}

void real_VdSetSystemCommandBufferGpuIdentifierAddress(PPCContext& ctx, uint8_t* base) {
    (void)base;
    auto& vd = vdGraphics();
    vd.gpuIdentifierAddr = uint32_t(ctx.r3.u64);
    RLOG("VdSetSystemCommandBufferGpuIdentifierAddress(0x%08X)", vd.gpuIdentifierAddr);
    ctx.r3.u32 = STATUS_SUCCESS;
}

void real_VdGetSystemCommandBuffer(PPCContext& ctx, uint8_t* base) {
    // (DWORD* out_base, DWORD* out_size) — buffer de sistema do kernel,
    // alocado de verdade (4 MiB) na janela virtual; o guest escreve nele.
    auto& vd = vdGraphics();
    if (vd.systemCmdBufferAddr == 0) {
        vd.systemCmdBufferSize = 0x400000;
        vd.systemCmdBufferAddr = uint32_t(virtWindow().alloc(vd.systemCmdBufferSize));
        if (vd.systemCmdBufferAddr == 0) {
            RLOG("VdGetSystemCommandBuffer: janela virtual esgotada (4 MiB)");
            ctx.r3.u32 = 0xC000009Au; // STATUS_INSUFFICIENT_RESOURCES
            return;
        }
        RLOG("VdGetSystemCommandBuffer: buffer real em 0x%08X (%u bytes)",
             vd.systemCmdBufferAddr, vd.systemCmdBufferSize);
    }
    if (validGuest(ctx.r3.u64, 4)) w32(base, ctx.r3.u64, vd.systemCmdBufferAddr);
    if (validGuest(ctx.r4.u64, 4)) w32(base, ctx.r4.u64, vd.systemCmdBufferSize);
    ctx.r3.u32 = STATUS_SUCCESS;
}

void real_VdInitializeRingBuffer(PPCContext& ctx, uint8_t* base) {
    (void)base;
    auto& vd = vdGraphics();
    vd.ringBufferBase = uint32_t(ctx.r3.u64);
    vd.ringBufferArg2 = uint32_t(ctx.r4.u64);
    // r3 = PA físico (MmGetPhysicalAddress), r4 = log2 do tamanho em
    // QUADWORDS (semântica real do console — xenia xboxkrnl_video.cc).
    RLOG("VdInitializeRingBuffer(base=0x%08X size_log2=0x%08X → %u bytes)",
         vd.ringBufferBase, vd.ringBufferArg2,
         vd.ringBufferArg2 < 24 ? (1u << (vd.ringBufferArg2 + 3)) : 0u);
    fh2::gpu::CommandProcessor::instance().onRingBufferInitialized(
        vd.ringBufferBase, vd.ringBufferArg2);
    ctx.r3.u32 = STATUS_SUCCESS;
}

void real_VdEnableRingBufferRPtrWriteBack(PPCContext& ctx, uint8_t* base) {
    (void)base;
    auto& vd = vdGraphics();
    vd.ringRptrAddr = uint32_t(ctx.r3.u64);
    vd.ringRptrArg2 = uint32_t(ctx.r4.u64);
    // r3 = PA do write-back, r4 = log2 do bloco (dwords lidos entre updates;
    // 6 no console — xenia command_processor EnableReadPointerWriteBack).
    RLOG("VdEnableRingBufferRPtrWriteBack(ptr=0x%08X block_log2=0x%08X)",
         vd.ringRptrAddr, vd.ringRptrArg2);
    fh2::gpu::CommandProcessor::instance().onRingPointerWriteBack(
        vd.ringRptrAddr, vd.ringRptrArg2);
    ctx.r3.u32 = STATUS_SUCCESS;
}

void real_VdSetGraphicsInterruptCallback(PPCContext& ctx, uint8_t* base) {
    (void)base;
    auto& vd = vdGraphics();
    vd.graphicsInterruptCb = uint32_t(ctx.r3.u64);
    vd.graphicsInterruptArg = uint32_t(ctx.r4.u64);
    // Registro REAL + thread de interrupção dedicada (vblank 60 Hz, TEB/
    // stack próprios — como no console). O D3D do título depende disso p/
    // fences/vsync/swap completion.
    RLOG("VdSetGraphicsInterruptCallback(cb=0x%08X arg=0x%08X) — thread de "
         "interrupção ativa",
         vd.graphicsInterruptCb, vd.graphicsInterruptArg);
    fh2::ppc::setGraphicsInterrupt(vd.graphicsInterruptCb,
                                   vd.graphicsInterruptArg);
    static std::thread s_interruptThread;
    static bool s_started = false;
    if (!s_started) {
        s_started = true;
        s_interruptThread = std::thread(fh2::ppc::graphicsInterruptLoop);
        s_interruptThread.detach();
    }
    ctx.r3.u32 = STATUS_SUCCESS;
}

void real_VdInitializeEngines(PPCContext& ctx, uint8_t* base) {
    (void)base;
    // O kernel inicializa os engines 2D/3D do Xenos — sem recursos
    // endereçáveis ao guest: sucesso real sem efeito observável aqui.
    RLOG("VdInitializeEngines()");
    ctx.r3.u32 = STATUS_SUCCESS;
}

void real_VdShutdownEngines(PPCContext& ctx, uint8_t* base) {
    (void)base;
    RLOG("VdShutdownEngines()");
    ctx.r3.u32 = STATUS_SUCCESS;
}

void real_VdPersistDisplay(PPCContext& ctx, uint8_t* base) {
    (void)base;
    ctx.r3.u32 = STATUS_SUCCESS;
}

void real_VdEnableDisableClockGating(PPCContext& ctx, uint8_t* base) {
    (void)base;
    vdGraphics().clockGating = uint32_t(ctx.r3.u64);
    ctx.r3.u32 = STATUS_SUCCESS;
}

void real_VdIsHSIOTrainingSucceeded(PPCContext& ctx, uint8_t* base) {
    (void)base;
    // Estado real do hardware pós-boot: treinamento HSIO concluído.
    ctx.r3.u32 = 1;
}

void real_VdRetrainEDRAM(PPCContext& ctx, uint8_t* base) {
    (void)base;
    ctx.r3.u32 = STATUS_SUCCESS;
}

void real_VdRetrainEDRAMWorker(PPCContext& ctx, uint8_t* base) {
    (void)base;
    ctx.r3.u32 = STATUS_SUCCESS;
}

void real_VdSetDisplayMode(PPCContext& ctx, uint8_t* base) {
    (void)base;
    vdGraphics().displayMode = uint32_t(ctx.r3.u64);
    ctx.r3.u32 = STATUS_SUCCESS;
}

void real_VdSetDisplayModeOverride(PPCContext& ctx, uint8_t* base) {
    (void)base;
    auto& vd = vdGraphics();
    vd.displayModeOverride = uint32_t(ctx.r3.u64);
    RLOG("VdSetDisplayModeOverride(0x%08X)", vd.displayModeOverride);
    ctx.r3.u32 = STATUS_SUCCESS;
}

void real_VdInitializeScalerCommandBuffer(PPCContext& ctx, uint8_t* base) {
    (void)base;
    auto& vd = vdGraphics();
    vd.scalerCbAddr = uint32_t(ctx.r3.u64);
    vd.scalerCbArg2 = uint32_t(ctx.r4.u64);
    RLOG("VdInitializeScalerCommandBuffer(0x%08X, 0x%08X)", vd.scalerCbAddr,
         vd.scalerCbArg2);
    ctx.r3.u32 = STATUS_SUCCESS;
}

// ============================================ criptografia Xe* (real)

// SHA-1 (FIPS 180-4) — implementação completa e auto-contida: o guest usa
// XeCryptSha na verificação de mídia no boot; o digest tem que ser o hash
// REAL dos bytes (o console calcula o mesmo valor).
namespace {

struct Sha1Ctx {
    uint32_t h[5];
    uint64_t bytes;
    uint8_t buf[64];
    size_t bufLen;
};

inline uint32_t sha1Rotl(uint32_t v, int n) {
    return (v << n) | (v >> (32 - n));
}

void sha1Init(Sha1Ctx& c) {
    c.h[0] = 0x67452301u;
    c.h[1] = 0xEFCDAB89u;
    c.h[2] = 0x98BADCFEu;
    c.h[3] = 0x10325476u;
    c.h[4] = 0xC3D2E1F0u;
    c.bytes = 0;
    c.bufLen = 0;
}

void sha1Block(Sha1Ctx& c, const uint8_t* p) {
    uint32_t w[80];
    for (int i = 0; i < 16; ++i) {
        w[i] = (uint32_t(p[i * 4]) << 24) | (uint32_t(p[i * 4 + 1]) << 16) |
               (uint32_t(p[i * 4 + 2]) << 8) | uint32_t(p[i * 4 + 3]);
    }
    for (int i = 16; i < 80; ++i) {
        w[i] = sha1Rotl(w[i - 3] ^ w[i - 8] ^ w[i - 14] ^ w[i - 16], 1);
    }
    uint32_t a = c.h[0], b = c.h[1], d = c.h[3], e = c.h[4];
    uint32_t cc = c.h[2];
    for (int i = 0; i < 80; ++i) {
        uint32_t f, k;
        if (i < 20) {
            f = (b & cc) | ((~b) & d);
            k = 0x5A827999u;
        } else if (i < 40) {
            f = b ^ cc ^ d;
            k = 0x6ED9EBA1u;
        } else if (i < 60) {
            f = (b & cc) | (b & d) | (cc & d);
            k = 0x8F1BBCDCu;
        } else {
            f = b ^ cc ^ d;
            k = 0xCA62C1D6u;
        }
        const uint32_t t = sha1Rotl(a, 5) + f + e + k + w[i];
        e = d;
        d = cc;
        cc = sha1Rotl(b, 30);
        b = a;
        a = t;
    }
    c.h[0] += a;
    c.h[1] += b;
    c.h[2] += cc;
    c.h[3] += d;
    c.h[4] += e;
}

void sha1Update(Sha1Ctx& c, const uint8_t* p, size_t n) {
    c.bytes += n;
    while (n > 0) {
        const size_t take = std::min(n, sizeof(c.buf) - c.bufLen);
        memcpy(c.buf + c.bufLen, p, take);
        c.bufLen += take;
        p += take;
        n -= take;
        if (c.bufLen == sizeof(c.buf)) {
            sha1Block(c, c.buf);
            c.bufLen = 0;
        }
    }
}

void sha1Final(Sha1Ctx& c, uint8_t out[20]) {
    const uint64_t bits = c.bytes * 8;
    const uint8_t pad = 0x80;
    sha1Update(c, &pad, 1);
    const uint8_t zero = 0;
    while (c.bufLen != 56) sha1Update(c, &zero, 1);
    uint8_t len[8];
    for (int i = 0; i < 8; ++i) len[i] = uint8_t(bits >> (56 - i * 8));
    // length bytes não contam para bytes/len de novo — escreve direto no buffer
    memcpy(c.buf + 56, len, 8);
    sha1Block(c, c.buf);
    c.bufLen = 0;
    for (int i = 0; i < 5; ++i) {
        out[i * 4] = uint8_t(c.h[i] >> 24);
        out[i * 4 + 1] = uint8_t(c.h[i] >> 16);
        out[i * 4 + 2] = uint8_t(c.h[i] >> 8);
        out[i * 4 + 3] = uint8_t(c.h[i]);
    }
}

} // namespace

void real_XeCryptSha(PPCContext& ctx, uint8_t* base) {
    // (PBYTE pb1, ULONG cb1, PBYTE pb2, ULONG cb2, PBYTE pb3, ULONG cb3,
    //  PBYTE pbDigest) — SHA-1 real sobre até 3 segmentos.
    Sha1Ctx c;
    sha1Init(c);
    const uint64_t ptrs[3] = {ctx.r3.u64, ctx.r5.u64, ctx.r7.u64};
    const uint32_t lens[3] = {ctx.r4.u32, ctx.r6.u32, ctx.r8.u32};
    for (int i = 0; i < 3; ++i) {
        if (lens[i] && validGuest(ptrs[i], lens[i])) {
            sha1Update(c, base + ptrs[i], lens[i]);
        }
    }
    uint8_t digest[20];
    sha1Final(c, digest);
    const uint64_t pDigest = ctx.r9.u64;
    if (validGuest(pDigest, 20)) {
        memcpy(base + pDigest, digest, 20);
    }
    ctx.r3.u32 = STATUS_SUCCESS;
}

// ================================================ Object Manager (Ob*)

namespace {

struct SymbolicLink {
    std::string name;
    std::string target;
};

std::mutex g_symM;
std::map<std::string, SymbolicLink> g_symLinks;  // chave: nome minúsculo

std::string lowerCopy(std::string s) {
    for (char& c : s) c = (char)tolower((unsigned char)c);
    return s;
}

} // namespace

void real_ObCreateSymbolicLink(PPCContext& ctx, uint8_t* base) {
    // (PANSI_STRING LinkName, PANSI_STRING Target) — registro REAL: opens
    // seguintes resolvem o nome pelo alvo (pathFromObjectAttributes).
    const std::string name = readAnsiString(base, ctx.r3.u64);
    const std::string target = readAnsiString(base, ctx.r4.u64);
    if (name.empty() || target.empty()) {
        RLOG("ObCreateSymbolicLink: nome/alvo vazio (name='%s' target='%s')",
             name.c_str(), target.c_str());
        ctx.r3.u32 = STATUS_INVALID_PARAMETER;
        return;
    }
    {
        std::lock_guard<std::mutex> lk(g_symM);
        g_symLinks[lowerCopy(name)] = SymbolicLink{name, target};
    }
    RLOG("ObCreateSymbolicLink('%s' → '%s')", name.c_str(), target.c_str());
    ctx.r3.u32 = STATUS_SUCCESS;
}

/** Resolve um caminho bruto do guest pelos symlinks registrados (nome cheio
 *  ou prefixo). Retorna true e reescreve `raw` quando resolve. */
static bool resolveObSymbolicLink(std::string& raw) {
    std::lock_guard<std::mutex> lk(g_symM);
    if (g_symLinks.empty()) return false;
    const std::string key = lowerCopy(raw);
    // 1) nome cheio
    auto it = g_symLinks.find(key);
    if (it != g_symLinks.end()) {
        raw = it->second.target;
        return true;
    }
    // 2) prefixo (nome + separador + resto)
    for (const auto& [k, link] : g_symLinks) {
        if (k.size() < key.size() && key.compare(0, k.size(), k) == 0 &&
            (key[k.size()] == '\\' || key[k.size()] == '/')) {
            raw = link.target + raw.substr(k.size());
            return true;
        }
    }
    return false;
}

// ============================================ I/O control (cache do título)

void real_NtDeviceIoControlFile(PPCContext& ctx, uint8_t* base) {
    // (HANDLE, EVENT, APC, Ctx, PIO_STATUS, CODE, In, InSize, Out, OutSize…)
    const uint32_t handle = ctx.r3.u32;
    const uint64_t pIoStatus = ctx.r7.u64;
    const uint32_t code = ctx.r8.u32;
    // ABI Xenon: args 9/10 vão no PARAMETER AREA do chamador — o guest grava
    // em [r1+84] (OutputBuffer) e [r1+92] (OutputLength) antes do bl (visto
    // nos call sites gerados: stw r8,84(r1) / stw r11,92(r1)). Slots de 8
    // bytes como no console; valores de 32 bits nos 4 bytes baixos.
    const uint64_t outBuf = g32(base, ctx.r1.u32 + 84);
    const uint32_t outLen = g32(base, ctx.r1.u32 + 92);
    GuestFile* f = fileGet(handle);
    static Throttle t;
    if (t.shouldLog(8, 256)) {
        RLOG("NtDeviceIoControlFile(handle=0x%08X %s code=0x%08X out=%08llX "
             "len=%u)", handle, f ? f->display.c_str() : "?", code,
             (unsigned long long)outBuf, outLen);
    }
    if (!f || !validGuest(pIoStatus, 8)) {
        ctx.r3.u32 = STATUS_INVALID_HANDLE;
        return;
    }
    if (f->dir || f->rawDevice) {
        // Cache0/Cache1 e o disco virtual: device control de PARTIÇÃO.
        w32(base, pIoStatus, 0);        // Status = SUCCESS
        if (code == k_IOCTL_DISK_GET_DRIVE_GEOMETRY) {
            // XMountUtilityDrive: out = {setores, bytes por setor}
            if (outLen >= 8 && validGuest(outBuf, 8)) {
                w32(base, outBuf, kCachePartitionSize / kCacheSectorSize);
                w32(base, outBuf + 4, kCacheSectorSize);
                w32(base, pIoStatus + 4, 8); // Information = bytes escritos
            }
            ctx.r3.u32 = STATUS_SUCCESS;
            return;
        }
        if (code == k_IOCTL_DISK_GET_PARTITION_INFO) {
            // XMountUtilityDrive: out = {início(8B)=0, tamanho(8B)=0xFF000}
            if (outLen >= 0x10 && validGuest(outBuf, 0x10)) {
                w32(base, outBuf, 0);
                w32(base, outBuf + 4, 0);
                w32(base, outBuf + 8, kCachePartitionSize);
                w32(base, outBuf + 12, 0);
                w32(base, pIoStatus + 4, 0x10);
            }
            ctx.r3.u32 = STATUS_SUCCESS;
            return;
        }
        // Código desconhecido em partição: request inválido (console real)
        w32(base, pIoStatus, 0xC0000002u);
        ctx.r3.u32 = 0xC0000002u; // STATUS_INVALID_DEVICE_REQUEST
        return;
    }
    w32(base, pIoStatus, 0xC0000002u); // STATUS_INVALID_DEVICE_REQUEST
    ctx.r3.u32 = 0xC0000002u;
}

// ============================================ cache do FS do título (D31)

void real_FscSetCacheElementCount(PPCContext& ctx, uint8_t* base) {
    (void)base;
    // O kernel do 360 dimensiona o cache de elementos do FS do título e
    // devolve ERROR_SUCCESS. Estado guardado (observável no diagnóstico).
    static std::atomic<uint32_t> s_cacheElements{0};
    s_cacheElements.store(ctx.r3.u32, std::memory_order_relaxed);
    RLOG("FscSetCacheElementCount(%u) — cache de elementos do título "
         "dimensionado",
         ctx.r3.u32);
    ctx.r3.u32 = 0; // ERROR_SUCCESS
}

void real_XamContentGetLicenseMask(PPCContext& ctx, uint8_t* base) {
    // Máscara de licenças de conteúdo do perfil: disco sem DLC = 0 (real).
    // Caminho síncrono (r4 = overlapped nulo): escreve a máscara. Com
    // overlapped: erro EXPLÍCITO (nunca sucesso falso).
    const uint64_t pMask = ctx.r3.u64;
    if (ctx.r4.u64 != 0) {
        RLOG("XamContentGetLicenseMask async (overlapped) = ERROR_NOT_SUPPORTED");
        ctx.r3.u32 = 50; // ERROR_NOT_SUPPORTED (Win32)
        return;
    }
    if (validGuest(pMask, 4)) {
        w32(base, pMask, 0);
    }
    ctx.r3.u32 = 0; // ERROR_SUCCESS
}

} // namespace fh2::kern

#endif // FH2_HAS_RECOMP
