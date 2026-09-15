// ppc_runtime.h — bootstrap do código recompilado (guest PowerPC → ARM64)
//
// O XenonRecomp gera funções C++ que operam sobre um PPCContext + memória
// base ABSOLUTA (base + endereço guest). Este runtime:
//  - Reserva a região virtual com mmap hint para base+0x82000000 ser válida
//  - Carrega a imagem guest quando o loader (backlog) estiver integrado
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
    PpcRuntime() = default;
    ~PpcRuntime(); // desmapeia a região guest

    PpcRuntime(const PpcRuntime&) = delete;
    PpcRuntime& operator=(const PpcRuntime&) = delete;

    /** Reserva a memória guest (idempotente). */
    bool initialize(fs::FsProvider* fs);

    /**
     * Loop principal do guest (thread dedicada).
     * Fase atual (v0.1): subsistemas prontos; execução do guest depende da
     * camada de kernel/IO (issue #16). Aguarda stop e responde a pause.
     */
    void run(gfx::GraphicsBackend* gfx, audio::AudioOutput* audio,
             input::InputState* input, fs::FsProvider* fs,
             std::atomic<bool>& stopRequested, std::atomic<bool>& paused);

private:
    uintptr_t memBase_ = 0;   // início da reserva (hint 0x02000000)
    uintptr_t guestBase_ = 0; // memBase_ + 0x82000000 (endereço guest 0)
};

} // namespace fh2::ppc
