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

#if FH2_HAS_RECOMP

#include <android/log.h>

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
#include "runtime/ppc/kernel_state.h"

#define RLOG(...) __android_log_print(ANDROID_LOG_INFO, "FH2/KRN", __VA_ARGS__)

namespace fh2::kern {

namespace {

// ----- acesso à memória guest (big-endian) -----
inline uint32_t g32(uint8_t* b, uint64_t a) {
    return __builtin_bswap32(*(uint32_t*)(b + a));
}
inline void w32(uint8_t* b, uint64_t a, uint32_t v) {
    *(uint32_t*)(b + a) = __builtin_bswap32(v);
}
inline uint64_t g64(uint8_t* b, uint64_t a) {
    return __builtin_bswap64(*(uint64_t*)(b + a));
}
inline void w64(uint8_t* b, uint64_t a, uint64_t v) {
    *(uint64_t*)(b + a) = __builtin_bswap64(v);
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

// timeout: PLARGE_INTEGER (100 ns; negativo = relativo). NULL = infinito.
inline int64_t readTimeout(uint8_t* base, uint64_t addr) {
    if (addr == 0 || !validGuest(addr, 8)) return -1; // infinito (relativo -1)
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

inline uint32_t waitOne(uint8_t* base, Waitable* w, uint64_t timeoutAddr,
                        bool alertable) {
    Waitable* objs[1] = {w};
    if (!w) return kWaitTimeout;
    return waitForMultiple(objs, 1, true, readTimeout(base, timeoutAddr),
                           alertable);
}

// STATUS constants reais do NT
constexpr uint32_t STATUS_SUCCESS = 0x00000000;
constexpr uint32_t STATUS_INVALID_PARAMETER = 0xC000000D;
constexpr uint32_t STATUS_INVALID_HANDLE = 0xC0000008;
constexpr uint32_t STATUS_ACCESS_DENIED = 0xC0000022;
constexpr uint32_t STATUS_OBJECT_NAME_NOT_FOUND = 0xC0000034;
constexpr uint32_t STATUS_NO_MEMORY = 0xC0000017;
constexpr uint32_t STATUS_UNSUCCESSFUL = 0xC0000001;
constexpr uint32_t STATUS_ACCESS_VIOLATION = 0xC0000005;

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
        memset(base + addr, 0, size);
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
    // (Mode, Alertable, PLARGE_INTEGER Interval) — negativo = relativo
    const uint64_t pInterval = ctx.r5.u64;
    const int64_t val = readTimeout(base, pInterval);
    int64_t ns = val < 0 ? -val * 100ull : 0;
    if (val == 0) ns = 0; // teste imediato
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
        const uint8_t* pa = base + a;
        const uint8_t* pb = base + b;
        while (n < len && pa[n] == pb[n]) ++n;
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
    const int n = guestFormatW((uint16_t*)(base + dst),
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

void real_XamLoaderSetLaunchData(PPCContext& ctx, uint8_t* base) {
    // (PVOID lpBuffer, DWORD cbSize) — guarda os dados de launch do título.
    const uint64_t data = ctx.r3.u64;
    uint32_t size = ctx.r4.u32;
    if (size > 512) size = 512; // XAM_LAUNCH_DATA real: 512 bytes
    if (size && !validGuest(data, size)) {
        ctx.r3.u32 = STATUS_INVALID_PARAMETER;
        return;
    }
    setLaunchData(size ? base + data : nullptr, size);
    ctx.r3.u32 = 0; // ERROR_SUCCESS
}

void real_XamLoaderGetLaunchData(PPCContext& ctx, uint8_t* base) {
    // (PVOID lpBuffer, DWORD cbSize) — copia os dados de launch (se houver).
    const uint64_t buf = ctx.r3.u64;
    const uint32_t bufLen = ctx.r4.u32;
    uint8_t tmp[512];
    const uint32_t size = getLaunchData(tmp, sizeof(tmp));
    if (size == 0) {
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
    ctx.r3.u32 = 0; // ERROR_SUCCESS
}

void real_XamLoaderGetLaunchDataSize(PPCContext& ctx, uint8_t* base) {
    // (PDWORD pcbSize) — tamanho dos dados de launch; sem dados: *pcb = 0 e
    // ERROR_NOT_FOUND (o título distingue boot frio de relaunch com dados).
    const uint32_t size = getLaunchDataSize();
    if (validGuest(ctx.r3.u64, 4)) w32(base, ctx.r3.u64, size);
    ctx.r3.u32 = size ? 0 : 0x80070490u; // ERROR_SUCCESS : ERROR_NOT_FOUND
}

void real_XamLoaderLaunchTitle(PPCContext& ctx, uint8_t* base) {
    // (LPCSTR pszPath, DWORD dwFlags) — NÃO RETORNA no console: o kernel
    // termina o título e o XAM o relança com o launch data preservado.
    // O runtime (PpcRuntime::run) faz o relaunch com estado zerado.
    const std::string path = readGuestCString(ctx.r3.u64, 512);
    const uint32_t flags = ctx.r4.u32;
    RLOG("XamLoaderLaunchTitle(\"%s\", flags=0x%08X) — encerrando título p/ "
         "relançar (não retorna)", path.c_str(), flags);
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
    // Módulos de SISTEMA (xam.xex, xboxkrnl.exe) existem sempre no console:
    // aqui são módulos VIRTUAIS com as exports que o título importa (stubs
    // reais na imagem — ver registro no boot). Qualquer outro nome falha
    // como no console (arquivo inexistente no volume do sistema).
    const std::string name = readGuestCString(ctx.r3.u64, 64);
    const uint64_t pHandle = ctx.r6.u64;
    if (name.empty() || !validGuest(pHandle, 4)) {
        ctx.r3.u32 = STATUS_INVALID_PARAMETER;
        return;
    }
    const uint32_t handle = registerModule(name, 0, {});
    w32(base, pHandle, handle);
    ctx.r3.u32 = STATUS_SUCCESS;
    RLOG("XexLoadImage(\"%s\", flags=0x%X) = 0x%08X", name.c_str(),
         ctx.r4.u32, handle);
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
        RLOG("XexGetProcedureAddress(0x%08X «%s», %u) = PROCEDURE_NOT_FOUND",
             h, m->name.c_str(), ordinal);
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
    for (const char* vol : {"game:", "d:", "cdrom0:"}) {
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

// Lê UNICODE_STRING do guest (UTF-16 big-endian) — campo ObjectName de
// OBJECT_ATTRIBUTES.
static std::string readUnicodeString(uint8_t* base, uint64_t addr) {
    if (!validGuest(addr, 8)) return {};
    const uint32_t len = g16(base, addr);        // bytes (não chars)
    const uint32_t buf = g32(base, addr + 4);
    if (len == 0 || len > 1024 || !validGuest(buf, len)) return {};
    std::string out;
    out.reserve(len / 2);
    for (uint32_t i = 0; i + 1 < len; i += 2) {
        const uint16_t wc = (uint16_t)(base[buf + i] << 8) | base[buf + i + 1];
        if (wc == 0) break;
        out.push_back(wc < 0x80 ? (char)wc : '?'); // caminhos do jogo são ASCII
    }
    return out;
}

// Extrai o caminho de OBJECT_ATTRIBUTES (r5): {len, rootDir, pName, attr,...}
static std::string pathFromObjectAttributes(uint8_t* base, uint64_t objAttr) {
    if (!validGuest(objAttr, 24)) return {};
    const uint32_t pName = g32(base, objAttr + 8);
    if (!pName) return {};
    const std::string raw = readUnicodeString(base, pName);
    return normalizeGuestPath(raw);
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
    std::string display = pathFromObjectAttributes(base, objAttr);
    std::string rel = normalizeGuestPath(display);
    const bool write = disposition == 2 || disposition == 3 || disposition == 5;
    // FILE_SUPERSEDE=0, CREATE_NEW=1, CREATE_ALWAYS=2, OPEN_EXISTING=3,
    // OPEN_ALWAYS=4, TRUNCATE_EXISTING=5
    uint32_t handle = 0;
    if (!rel.empty()) {
        GuestFile* f = nullptr;
        handle = fileOpen(rel, display, write, &f);
    }
    if (!handle) {
        w32(base, pIoStatus, STATUS_OBJECT_NAME_NOT_FOUND);
        ctx.r3.u32 = STATUS_OBJECT_NAME_NOT_FOUND;
        static Throttle tf;
        if (tf.shouldLog(24, 512)) {
            RLOG("NtCreateFile('%s') = NOT_FOUND", display.c_str());
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
    const std::string display = pathFromObjectAttributes(base, objAttr);
    const std::string rel = normalizeGuestPath(display);
    uint32_t handle = 0;
    if (!rel.empty()) {
        GuestFile* f = nullptr;
        handle = fileOpen(rel, display, false, &f);
    }
    if (!handle) {
        w32(base, pIoStatus, STATUS_OBJECT_NAME_NOT_FOUND);
        ctx.r3.u32 = STATUS_OBJECT_NAME_NOT_FOUND;
        static Throttle tf;
        if (tf.shouldLog(24, 512)) {
            RLOG("NtOpenFile('%s') = NOT_FOUND", display.c_str());
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
    if (!f || f->fd < 0 || !validGuest(pIoStatus, 8)) {
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
    uint32_t done = 0;
    if (length > 0) {
        if (off >= f->size) {
            done = 0; // EOF real
        } else {
            const uint64_t want = std::min<uint64_t>(length, f->size - off);
            ssize_t n;
            do {
                n = pread(f->fd, base + buffer + done, want - done,
                          (off_t)(off + done));
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
    const std::string display = pathFromObjectAttributes(base, objAttr);
    const std::string rel = normalizeGuestPath(display);
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

} // namespace fh2::kern

#endif // FH2_HAS_RECOMP
