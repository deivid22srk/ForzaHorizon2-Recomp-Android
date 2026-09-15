// ppc_runtime.h — bootstrap do código recompilado (guest PowerPC → ARM64)
//
// O XenonRecomp gera funções C++ que operam sobre um PPCContext + memória
// base mapeada em 0x82000000. Este runtime:
//  - Aloca a memória guest (imagem 23 MB + heap) e carrega o XEX do usuário
//  - Constrói a perfect hash table de funções (ppc_func_mapping)
//  - Fornece o ponto de entrada da thread principal do guest
#pragma once

#include <atomic>
#include <cstdint>

namespace fh2 { namespace fs { class FsProvider; } }
namespace fh2 { namespace gfx { class GraphicsBackend; } }
namespace fh2 { namespace audio { class AudioOutput; } }
namespace fh2 { namespace input { struct InputState; } }

namespace fh2::ppc {

class PpcRuntime {
public:
    /** Aloca memória guest e prepara o mapeamento de funções. */
    bool initialize(fs::FsProvider* fs);

    /**
     * Loop principal do guest (thread dedicada).
     * Fase atual (v0.1): subsistemas prontos; execução do guest depende da
     * camada de kernel/IO (backlog). Aguarda stop e responde a pause.
     */
    void run(gfx::GraphicsBackend* gfx, audio::AudioOutput* audio,
             input::InputState* input, fs::FsProvider* fs,
             std::atomic<bool>& stopRequested, std::atomic<bool>& paused);

private:
    uint8_t* guestMemory_ = nullptr;
    size_t guestMemorySize_ = 0;
    bool codeLoaded_ = false;
};

} // namespace fh2::ppc
