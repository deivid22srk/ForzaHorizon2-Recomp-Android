// ppc_runtime.h — bootstrap do código recompilado (guest PowerPC → ARM64)
//
// O XenonRecomp gera funções C++ que recebem `uint8_t* base` e acessam a
// memória guest como *(base + endereço_guest). O runtime:
//  - Reserva uma região virtual contígua de ~2,25 GB (MAP_NORESERVE) capaz
//    de cobrir todo o espaço endereçável usado pelo guest (0 .. 0x82000000 +
//    imagem + margem). A hint 0x02000000 é apenas conveniência de depuração:
//    como `base` é um ponteiro de runtime passado por parâmetro em TODA
//    função gerada, o modelo base+addr é válido em qualquer endereço —
//    kernels Android que não honram a hint continuam suportados.
//  - Carrega e decodifica o XEX2 do jogo (default.xex) na memória guest via
//    xex_loader (AES-128 + descompressão + patch de imports consistente com
//    o código gerado).
//  - Fornece o ponto de entrada da thread principal do guest
#pragma once

#include <atomic>
#include <cstdint>

#include "runtime/ppc/xex_loader.h"

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

    /** Reserva a memória guest e carrega o XEX do jogo (idempotente). */
    bool initialize(fs::FsProvider* fs);

    /**
     * Loop principal do guest (thread dedicada).
     * Fase atual (v0.1): subsistemas prontos + imagem guest carregada; a
     * execução do entry point depende da resolução de imports/HLE (issue #16).
     * Aguarda stop e responde a pause.
     */
    void run(gfx::GraphicsBackend* gfx, audio::AudioOutput* audio,
             input::InputState* input, fs::FsProvider* fs,
             std::atomic<bool>& stopRequested, std::atomic<bool>& paused);

private:
    uintptr_t memBase_ = 0;   // início da reserva (endereço guest 0)
    uintptr_t guestBase_ = 0; // memBase_ + 0x82000000 (início da imagem)
    XexImageInfo imageInfo_;  // base/entry/tamanho reais do XEX carregado
};

} // namespace fh2::ppc
