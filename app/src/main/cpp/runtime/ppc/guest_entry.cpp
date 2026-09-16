// guest_entry.cpp — chamada REAL do guest (código gerado pelo XenonRecomp)
//
// Contratos do código gerado (recomp/runtime/ppc_context.h):
//   PPCFunc = void f(PPCContext& ctx, uint8_t* base)
//   Chamada indireta: *(PPCFunc**)(base + PPC_IMAGE_BASE + PPC_IMAGE_SIZE +
//                     (addrGuest - PPC_CODE_BASE) * 2)   ← tabela mágica
//   r1 = stack pointer guest, r13 = TLS do thread, entry em PPCFuncMappings.
#if FH2_HAS_RECOMP

#include "guest_entry.h"

#include <android/log.h>

#include <chrono>
#include <csetjmp>
#include <cstring>
#include <mutex>
#include <thread>
#include <unordered_map>

#include "ppc_config.h"
#include "ppc_context.h"
#include "ppc_recomp_shared.h"
#include "runtime/ppc/kernel_real.h"
#include "runtime/ppc/kernel_state.h"
#include "runtime/ppc/xex_loader.h"

#define GLOG(...) __android_log_print(ANDROID_LOG_INFO, "FH2/GUEST", __VA_ARGS__)

namespace fh2::ppc {

namespace {

std::mutex g_mapM;
std::unordered_map<uint32_t, PPCFunc*> g_funcMap;
bool g_mapBuilt = false;

PPCFunc* lookup(uint32_t addr) {
    std::lock_guard<std::mutex> lk(g_mapM);
    if (!g_mapBuilt) {
        for (PPCFuncMapping* m = PPCFuncMappings; m->host; ++m) {
            g_funcMap.emplace((uint32_t)m->guest, m->host);
        }
        g_mapBuilt = true;
        GLOG("mapa de funções guest: %zu entradas", g_funcMap.size());
    }
    auto it = g_funcMap.find(addr);
    return it == g_funcMap.end() ? nullptr : it->second;
}

// ----------------------------------------------------------------------
// TEB + TLS do módulo: no Xenon, r13 aponta para o TEB da thread (SEMPRE
// válido — o kernel o aloca na criação) e a imagem TLS do módulo é anexada
// após a área de sistema. O template vive na imagem (tlsBaseAddr, VA) com
// tlsDataSize bytes inicializados e zero-init até tlsTotalSize.
// Campos de sistema gravados pelo kernel real e reproduzidos aqui:
//   +0x20 ClientId.UniqueProcess (titleId)   +0x24 ClientId.UniqueThread
//   +0x28 StackBase (topo)                   +0x2C StackLimit (base)
uint64_t setupTls(uint8_t* base, const XexImageInfo& img,
                  uint32_t threadId, uint64_t stackTop, uint64_t stackLimit) {
    constexpr uint32_t kTebSystemSize = 0x1000;
    const uint32_t tlsTotal = (img.tlsTotalSize + 31u) & ~31u;
    const uint32_t blockSize = kTebSystemSize + tlsTotal;
    const uint64_t block = kern::heap().alloc(blockSize, 32);
    if (!block) {
        GLOG("TEB/TLS: heap esgotado (%u bytes) — r13 = 0", blockSize);
        return 0;
    }
    uint8_t* host = base + block;
    memset(host, 0, blockSize);
    // template TLS inicializado (dados vivos na imagem carregada)
    if (img.tlsDataSize && img.tlsBaseAddr >= PPC_IMAGE_BASE &&
        img.tlsBaseAddr + img.tlsDataSize <= PPC_IMAGE_BASE + PPC_IMAGE_SIZE) {
        memcpy(host + kTebSystemSize, base + img.tlsBaseAddr, img.tlsDataSize);
    }
    // campos de sistema do TEB (mesmos valores que o kernel real grava)
    *(uint32_t*)(host + 0x20) = __builtin_bswap32(kern::currentTitleId());
    *(uint32_t*)(host + 0x24) = __builtin_bswap32(threadId);
    *(uint32_t*)(host + 0x28) = __builtin_bswap32((uint32_t)stackTop);
    *(uint32_t*)(host + 0x2C) = __builtin_bswap32((uint32_t)stackLimit);
    return block;
}

// Encerramento é feito por kern::terminateCurrentThread (longjmp direto).

} // namespace

uint64_t populateMagicTable(uint8_t* guestMemZero) {
    PPCFunc** table = reinterpret_cast<PPCFunc**>(
        guestMemZero + PPC_IMAGE_BASE + PPC_IMAGE_SIZE);
    uint64_t count = 0;
    for (PPCFuncMapping* m = PPCFuncMappings; m->host; ++m) {
        const uint64_t idx2 =
            (uint64_t)((uint32_t)m->guest - (uint32_t)PPC_CODE_BASE) * 2ull;
        table[idx2 / 8] = m->host;
        ++count;
    }
    GLOG("tabela mágica de funções populada: %llu entradas em 0x%08llX",
         (unsigned long long)count,
         (unsigned long long)(PPC_IMAGE_BASE + PPC_IMAGE_SIZE));
    return count;
}

void* resolveGuestFunc(uint32_t guestAddr) {
    return (void*)lookup(guestAddr);
}

void guestThreadBody(kern::GuestThread& t, uint8_t* base,
                     const XexImageInfo& img) {
    // Espera SUSPENSA até KeResumeThread (ou stop).
    {
        std::unique_lock<std::mutex> lk(t.startM);
        t.startCv.wait(lk, [&] { return t.started || kern::stopRequested(); });
    }
    if (kern::stopRequested() && !t.started) return;

    PPCContext ctx{};
    ctx.msr = 0x200A000;

    // Identidade da thread no kernel (ClientId.UniqueThread) — deve bater com
    // o TEB+0x24 gravado abaixo, para que comparações de ownership de
    // critical sections do guest sejam idênticas às do kernel real.
    kern::setHostThreadId(t.id);

    // Stack própria (janela virtual do kernel guest)
    const uint64_t stackSize = t.stackSize ? t.stackSize : 0x40000;
    t.stackGuest = kern::virtWindow().alloc(stackSize + 0x10000);
    if (!t.stackGuest) {
        GLOG("thread %u: sem memória para stack (%llu bytes)", t.handle,
             (unsigned long long)stackSize);
        return;
    }
    ctx.r1.u64 = (t.stackGuest + stackSize) & ~0xFull; // topo, 16-alinhado
    GLOG("thread %u (id=%u): start=0x%08llX ctx=0x%08llX stack=0x%08llX+%lluk",
         t.handle, t.id, (unsigned long long)t.startRoutine,
         (unsigned long long)t.startContext,
         (unsigned long long)t.stackGuest,
         (unsigned long long)(stackSize >> 10));

    ctx.r13.u64 = setupTls(base, img, t.id,
                           (t.stackGuest + stackSize) & ~0xFull,
                           t.stackGuest);

    PPCFunc* fn = lookup((uint32_t)t.startRoutine);
    if (!fn) {
        GLOG("thread %u: startRoutine 0x%08llX sem função recompilada — "
             "abortando thread", t.handle,
             (unsigned long long)t.startRoutine);
        kern::virtWindow().free(t.stackGuest);
        return;
    }

    // Argumento da thread (LPTHREAD_START_ROUTINE: r3 = startContext)
    ctx.r3.u64 = t.startContext;
    // LR sentinel: retorno = fim da thread
    ctx.lr = 0;

    jmp_buf env;
    kern::setUnwindTarget(&env);
    t.envValid = true;
    int jump = setjmp(env);
    if (jump == 0) {
        fn(ctx, base); // EXECUÇÃO REAL da thread guest
        GLOG("thread %u: rotina retornou (lr=0x%08llX)", t.handle,
             (unsigned long long)ctx.lr);
    } else if (jump == 2) {
        GLOG("thread %u: terminada via ExTerminateThread", t.handle);
    } else {
        GLOG("thread %u: unwind por stop", t.handle);
    }
    t.envValid = false;
    kern::setUnwindTarget(nullptr);
    if (t.stackGuest) kern::virtWindow().free(t.stackGuest);
}

void runGuestMain(uint32_t entryAddr, uint8_t* base, const XexImageInfo& img,
                  std::atomic<bool>& stopRequested) {
    PPCContext ctx{};
    ctx.msr = 0x200A000;
    kern::setHostThreadId(1); // thread principal do título = kernel id 1

    // Stack da thread principal: 16 MB na janela virtual
    const uint64_t kMainStackSize = 0x1000000ull;
    const uint64_t stack = kern::virtWindow().alloc(kMainStackSize);
    if (!stack) {
        GLOG("sem memória para a stack principal — não executando guest");
        return;
    }
    ctx.r1.u64 = (stack + kMainStackSize - 0x1000) & ~0xFull;
    const uint64_t stackTop = ctx.r1.u64;
    GLOG("entry 0x%08X: stack 0x%08llX..0x%08llX, %u bytes de TLS (template)",
         entryAddr, (unsigned long long)stack,
         (unsigned long long)(stack + kMainStackSize), img.tlsTotalSize);

    ctx.r13.u64 = setupTls(base, img, /*threadId=*/1, stackTop, stack);
    if (ctx.r13.u64) {
        GLOG("TEB principal: r13=0x%08llX (%u bytes, id=1)",
             (unsigned long long)ctx.r13.u64, 0x1000 + img.tlsTotalSize);
    }

    PPCFunc* entry = lookup(entryAddr);
    if (!entry) {
        GLOG("entry 0x%08X não existe em PPCFuncMappings — o XEX não "
             "corresponde ao código recompilado", entryAddr);
        return;
    }

    jmp_buf env;
    kern::setUnwindTarget(&env);
    const int jump = setjmp(env);
    if (jump == 0) {
        GLOG("chamando entry do guest (EXECUÇÃO REAL)…");
        entry(ctx, base);
        fh2::kern::traceDump("guest main retornou");
        GLOG("guest main RETORNOU (lr=0x%08llX, r3=0x%08X) — boot concluído "
             "ou main saiu", (unsigned long long)ctx.lr,
             (unsigned)ctx.r3.u32);
    } else if (jump == 2) {
        GLOG("guest main terminado via ExTerminateThread/HalReturnToFirmware");
    } else {
        GLOG("guest main interrompido por stop do usuário");
    }
    kern::setUnwindTarget(nullptr);

    // Threads guest continuam vivas até stop; em stop elas recebem o unwind
    // pelos waits/delays (kernel_state::requestStop). Espera um ciclo para
    // o encerramento limpo antes de liberar quem depende da memória.
    while (!stopRequested) {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    const size_t alive = kern::joinAll(2000);
    if (alive > 0) {
        GLOG("%zu thread(s) guest não terminaram no stop (unwind no próximo "
             "ponto de bloqueio)", alive);
    }
}

} // namespace fh2::ppc

#endif // FH2_HAS_RECOMP
