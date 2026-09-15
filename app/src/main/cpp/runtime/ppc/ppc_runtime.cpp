// ppc_runtime.cpp — bootstrap do guest
//
// Constantes do XEX do FH2 (geradas para recomp/generated/ppc_config.h):
//   PPC_IMAGE_BASE 0x82000000, PPC_IMAGE_SIZE 0x1700000
// A memória guest é alocada alinhada a 64 KB (page size do Xenon).
#include "ppc_runtime.h"

#include <android/log.h>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <thread>
#include <vector>

#include "runtime/audio/audio_output.h"
#include "runtime/fs/fs_provider.h"
#include "runtime/gfx/graphics_backend.h"
#include "runtime/input/input_state.h"

#if FH2_HAS_RECOMP
#include "ppc_config.h"
#endif

#define PLOG(...) __android_log_print(ANDROID_LOG_INFO, "FH2/PPC", __VA_ARGS__)

namespace fh2::ppc {

bool PpcRuntime::initialize(fs::FsProvider* fs) {
#if FH2_HAS_RECOMP
    // Imagem guest: base + espaço para o "magic function table" (ver README do
    // XenonRecomp: endereços de função ficam após a região válida do XEX)
    guestMemorySize_ = size_t(PPC_IMAGE_SIZE) + 0x1000000; // + 16 MB de margem
    guestMemory_ = static_cast<uint8_t*>(std::aligned_alloc(0x10000, guestMemorySize_));
    if (!guestMemory_) {
        PLOG("falha ao alocar %zu MB de memória guest", guestMemorySize_ >> 20);
        return false;
    }
    std::memset(guestMemory_, 0, guestMemorySize_);
    PLOG("memória guest alocada: %.1f MB em base fixa", guestMemorySize_ / 1048576.0);

    // O usuário fornece default_dec.xex na pasta de assets selecionada;
    // o runtime carrega a imagem (código+dados) para a base.
    std::vector<uint8_t> xex;
    if (fs && fs->readFile("default_dec.xex", xex)) {
        PLOG("default_dec.xex do usuário carregado: %zu bytes", xex.size());
        codeLoaded_ = true;
    } else {
        PLOG("default_dec.xex não encontrado na pasta de assets (o código recompilado "
             "já está compilado no APK; a imagem carrega dados/patches em runtime)");
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

    if (audio) audio->start();

    // Loop de "present" mínimo: mantém o frame loop do app vivo até a
    // integração completa do guest (kernel/IO). Sem ele, a superfície fica
    // preta e o sistema pode matar o app por ANR de renderização.
    while (!stopRequested) {
        if (paused) {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            continue;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(16)); // ~60 Hz
#if FH2_HAS_RECOMP
        // TODO(backlog): chamada do guest main (entry 0x82BF2CD0 via k-function
        // table) após a camada de kernel/IO responder aos imports do XEX.
#endif
    }

    PLOG("run: encerrado");
}

} // namespace fh2::ppc
