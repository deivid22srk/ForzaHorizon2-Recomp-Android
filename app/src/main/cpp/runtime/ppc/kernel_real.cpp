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
#include <cstring>
#include <thread>

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
    return guestMem() && a != 0 && a + n <= guestMemBytes();
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

thread_local uint64_t t_kthreadBlock = 0; // bloco real p/ o KTHREAD opaco

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
    // (&Base, ZeroBits, &Size, Type, Protect) — 5 args
    const uint64_t pBase = ctx.r3.u64, pSize = ctx.r5.u64;
    uint64_t wantBase = 0, size = 0;
    if (validGuest(pBase, 4)) wantBase = g32(base, pBase);
    if (validGuest(pSize, 4)) size = g32(base, pSize);
    if (size == 0) {
        ctx.r3.u32 = STATUS_INVALID_PARAMETER;
        return;
    }
    uint64_t addr = wantBase ? virtWindow().allocAt(wantBase, size)
                             : virtWindow().alloc(size);
    if (!addr) {
        ctx.r3.u32 = STATUS_NO_MEMORY;
        RLOG("NtAllocateVirtualMemory(%s%llu) falhou (espaço)",
             wantBase ? "base fixa " : "", (unsigned long long)size);
        return;
    }
    if (validGuest(pBase, 4)) w32(base, pBase, (uint32_t)addr);
    if (validGuest(pSize, 4)) w32(base, pSize, (uint32_t)size);
    ctx.r3.u32 = STATUS_SUCCESS;
    RLOG("NtAllocateVirtualMemory(base=%llu size=%llu) = 0x%08X",
         (unsigned long long)wantBase, (unsigned long long)size,
         (unsigned)addr);
}

void real_NtFreeVirtualMemory(PPCContext& ctx, uint8_t* base) {
    const uint64_t pBase = ctx.r3.u64;
    if (validGuest(pBase, 4)) {
        const uint64_t addr = g32(base, pBase);
        virtWindow().free(addr);
    }
    ctx.r3.u32 = STATUS_SUCCESS;
}

void real_MmAllocatePhysicalMemoryEx(PPCContext& ctx, uint8_t* base) {
    // (Type, Size, MinAddr, MaxAddr, Alignment, Tag) — 6 args
    const uint64_t size = ctx.r4.u64;
    const uint64_t align = ctx.r8.u64 ? ctx.r8.u64 : 0x1000;
    const uint64_t addr = virtWindow().alloc((size + align - 1) & ~(align - 1));
    ctx.r3.u64 = addr;
    if (!addr) {
        RLOG("MmAllocatePhysicalMemoryEx(size=%llu align=%llu) = NULL",
             (unsigned long long)size, (unsigned long long)align);
    }
}

void real_MmFreePhysicalMemory(PPCContext& ctx, uint8_t* base) {
    virtWindow().free(ctx.r4.u64);
}

void real_MmGetPhysicalAddress(PPCContext& ctx, uint8_t* base) {
    // Identidade: VA guest == "físico" guest (nosso modelo de memória)
    ctx.r3.u64 = ctx.r3.u64;
}

void real_MmQueryAllocationSize(PPCContext& ctx, uint8_t* base) {
    const uint64_t addr = ctx.r3.u64;
    const uint64_t sz = virtWindow().blockSize(addr);
    ctx.r3.u64 = sz;
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
    closeHandle((uint32_t)ctx.r3.u64);
    ctx.r3.u32 = STATUS_SUCCESS;
}

// ----------------------------------------------------- critical sections

void real_RtlInitializeCriticalSection(PPCContext& ctx, uint8_t* base) {
    Waitable* w = waitable(ctx.r3.u64, true, WaitKind::Mutant);
    if (w) {
        std::lock_guard<std::mutex> lk(waitMutex());
        w->signaled = true; // livre
        w->kind = WaitKind::Mutant;
    }
    ctx.r3.u32 = STATUS_SUCCESS;
}

void real_RtlInitializeCriticalSectionAndSpinCount(PPCContext& ctx, uint8_t* base) {
    real_RtlInitializeCriticalSection(ctx, base);
}

void real_RtlEnterCriticalSection(PPCContext& ctx, uint8_t* base) {
    Waitable* w = waitable(ctx.r3.u64, true, WaitKind::Mutant);
    if (w) {
        Waitable* objs[1] = {w};
        for (;;) {
            const uint32_t r = waitForMultiple(objs, 1, true, -1, false);
            if (stopRequested()) {
                jmp_buf* env = unwindTarget();
                if (env) longjmp(*env, 1);
                break; // stop sem alvo de unwind: não trava o encerramento
            }
            // mutex: sinalizado → consumir (autoReset faz o papel de acquire)
            if (r == kWaitSignaled0) break;
        }
    }
}

void real_RtlLeaveCriticalSection(PPCContext& ctx, uint8_t* base) {
    setEvent(waitable(ctx.r3.u64, true, WaitKind::Mutant));
}

void real_RtlTryEnterCriticalSection(PPCContext& ctx, uint8_t* base) {
    Waitable* w = waitable(ctx.r3.u64, true, WaitKind::Mutant);
    ctx.r3.u32 = 0;
    if (!w) return;
    std::lock_guard<std::mutex> lk(waitMutex());
    if (w->signaled) {
        w->signaled = false;
        ctx.r3.u32 = 1;
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
    jmp_buf* env = unwindTarget();
    if (env) longjmp(*env, 3);
}

void real_KeBugCheckEx(PPCContext& ctx, uint8_t* base) {
    RLOG("KeBugCheckEx(code=0x%08X, %016llX, %016llX, %016llX) — bugcheck REAL",
         (unsigned)ctx.r3.u32, (unsigned long long)ctx.r4.u64,
         (unsigned long long)ctx.r5.u64, (unsigned long long)ctx.r6.u64);
    jmp_buf* env = unwindTarget();
    if (env) longjmp(*env, 3);
}

void real_HalReturnToFirmware(PPCContext& ctx, uint8_t* base) {
    RLOG("HalReturnToFirmware(%u) — guest pediu desligamento/reboot",
         (unsigned)ctx.r3.u32);
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

void real_XamUserGetSigninState(PPCContext& ctx, uint8_t* base) {
    ctx.r3.u32 = 0; // sem perfil assinado (estado real v1: offline)
}

} // namespace fh2::kern

#endif // FH2_HAS_RECOMP
