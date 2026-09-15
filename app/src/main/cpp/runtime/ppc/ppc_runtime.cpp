// ppc_runtime.cpp — bootstrap do guest
//
// Modelo de memória (crítico): o código gerado pelo XenonRecomp endereça a
// memória guest de forma ABSOLUTA — `PPC_LOAD_U32(x) = *(base + x)` onde x é o
// endereço guest (ex.: 0x82000000 + offset). Portanto o runtime deve mapear a
// região de forma que `memBase + PPC_IMAGE_BASE` aponte para o buffer real.
// Reservamos com mmap hint em 0x02000000 (64-bit Android honra hints livres):
// guest fica em 0x84000000.. e ainda sobra espaço acima para a "magic function
// table" do XenonRecomp (funções mapeadas após a região válida do XEX).
// MAP_NORESERVE: 2,25 GB de endereçamento virtual, commit só no uso real.
#include "ppc_runtime.h"

#include <android/log.h>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <thread>
#include <vector>

#include <sys/mman.h>

#include "runtime/audio/audio_output.h"
#include "runtime/fs/fs_provider.h"
#include "runtime/gfx/graphics_backend.h"
#include "runtime/input/input_state.h"

#if FH2_HAS_RECOMP
#include "ppc_config.h"
#endif

#define PLOG(...) __android_log_print(ANDROID_LOG_INFO, "FH2/PPC", __VA_ARGS__)

namespace fh2::ppc {

// Endereço virtual do hint: guest base = hint + 0x82000000 = 0x84000000.
static constexpr uintptr_t kMemBaseHint = 0x02000000ull;
// Espaço acima do fim da imagem: heap guest + magic function table
// (endereços de função vivem após a região válida do XEX — ver README do
// XenonRecomp / ppc_config.h: PPC_CODE_BASE).
static constexpr size_t kMemImageSize =
#if FH2_HAS_RECOMP
    static_cast<size_t>(PPC_IMAGE_SIZE);
#else
    0x1700000;
#endif
static constexpr size_t kMemMargin = 0x1000000;      // 16 MB de margem
static constexpr size_t kMemTotal = 0x82000000ull + kMemImageSize + kMemMargin;

PpcRuntime::~PpcRuntime() {
    if (memBase_) {
        munmap(reinterpret_cast<void*>(memBase_), kMemTotal);
        memBase_ = 0;
    }
}

bool PpcRuntime::initialize(fs::FsProvider* fs) {
#if FH2_HAS_RECOMP
    if (memBase_) return true; // idempotente

    void* mapped = mmap(reinterpret_cast<void*>(kMemBaseHint), kMemTotal,
                        PROT_READ | PROT_WRITE,
                        MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);
    if (mapped == MAP_FAILED || reinterpret_cast<uintptr_t>(mapped) != kMemBaseHint) {
        PLOG("mmap hint 0x%llX falhou (obtido %p) — modelo base+addr inviável neste device",
             (unsigned long long)kMemBaseHint, mapped == MAP_FAILED ? nullptr : mapped);
        if (mapped != MAP_FAILED) munmap(mapped, kMemTotal);
        return false;
    }
    memBase_ = reinterpret_cast<uintptr_t>(mapped);
    guestBase_ = memBase_ + 0x82000000ull;
    PLOG("memória guest reservada: %zu MB (hint fixa) — guest em 0x%08llX",
         kMemTotal >> 20, (unsigned long long)guestBase_);

    // A imagem (código+dados) do usuário não é carregada nesta fase: o XEX
    // distribuído em disco é retail-encrypted; a imagem descriptografada será
    // embutida pelo CI como asset do APK (BACKLOG: loader de imagem + patches).
    // Aqui apenas verificamos a presença dos arquivos esperados na pasta SAF.
    if (fs) {
        std::vector<uint8_t> probe;
        if (fs->readFile("default.xex", probe, /*maxBytes=*/1)) {
            PLOG("pasta de assets: default.xex presente (%zu+ bytes)", probe.size());
        } else {
            PLOG("pasta de assets: default.xex não encontrado (o jogo não executará "
                 "nesta fase; loader de imagem é backlog)");
        }
    }
    return true;
#else
    PLOG("build sem código recompilado (FH2_HAS_RECOMP=0) — modo shell");
    (void)fs;
    return true;
#endif
}

void PpcRuntime::run(gfx::GraphicsBackend* gfx, audio::AudioOutput* audio,
                     input::InputState* input, fs::FsProvider* fs,
                     std::atomic<bool>& stopRequested, std::atomic<bool>& paused) {
    PLOG("run: subsistemas gfx=%s audio=%s input=ok fs=%s",
         gfx ? gfx->name() : "-", audio ? "aaudio" : "-", fs ? "ok" : "-");
    (void)input;
    (void)fs;

    if (audio) audio->start();

    // Loop de "present" mínimo: mantém o frame loop do app vivo até a
    // integração completa do guest (kernel/IO — issue #16). Sem ele, a
    // superfície fica preta e o sistema pode matar o app por ANR.
    int targetFps = gfx ? gfx->fpsTarget() : 60;
    auto frameInterval = std::chrono::milliseconds(1000 / (targetFps > 0 ? targetFps : 60));
    while (!stopRequested) {
        if (paused) {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            continue;
        }
        std::this_thread::sleep_for(frameInterval);
#if FH2_HAS_RECOMP
        // TODO(backlog #16): chamada do guest main (entry 0x82BF2CD0 via
        // ppc_func_mapping) após kernel/IO responder aos imports.
#endif
    }

    PLOG("run: encerrado");
}

} // namespace fh2::ppc
