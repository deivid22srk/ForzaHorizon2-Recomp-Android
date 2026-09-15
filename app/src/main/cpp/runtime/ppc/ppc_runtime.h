// ppc_runtime.h — bootstrap do código recompilado (guest PowerPC → ARM64)
//
// O XenonRecomp gera funções C++ que recebem `uint8_t* base` e acessam a
// memória guest como *(base + endereço_guest). O runtime:
//  - Reserva uma região virtual contígua cobrindo o espaço guest usado
//    (0 .. stack principal + margem). A hint 0x02000000 é apenas
//    conveniência de depuração: `base` é passado por parâmetro em TODA
//    função gerada, o modelo base+addr é válido em qualquer endereço.
//  - Carrega e decodifica o XEX2 do jogo (default.xex) na memória guest via
//    xex_loader (AES-128 + descompressão + patch de imports consistente com
//    o código gerado) e extrai TLS/stack/heap/title id dos headers.
//  - Popula a TABELA MÁGICA de funções (chamadas indiretas geradas:
//    vtables, function pointers, imports) logo após a imagem.
//  - Inicializa heap guest + janela virtual (kernel HLE — guest_alloc).
//  - Executa o entry point REAL do jogo (guest_entry) na thread principal
//    e nas threads guest criadas via ExCreateThread.
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
    ~PpcRuntime(); // desmapeia a região guest (se o guest não estiver vivo)

    PpcRuntime(const PpcRuntime&) = delete;
    PpcRuntime& operator=(const PpcRuntime&) = delete;

    /** Reserva a memória guest, carrega o XEX, popula a tabela mágica e
     *  prepara heap/janela virtual (idempotente). */
    bool initialize(fs::FsProvider* fs);

    /**
     * Execução REAL do guest (thread dedicada): monta o PPCContext da
     * thread principal e chama o entry point do XEX. Bloqueia até o guest
     * terminar ou stopRequested — os waits HLE fazem o unwind do stop.
     */
    void run(gfx::GraphicsBackend* gfx, audio::AudioOutput* audio,
             input::InputState* input, fs::FsProvider* fs,
             std::atomic<bool>& stopRequested, std::atomic<bool>& paused);

    /** true quando o guest chegou a EXECUTAR código (sem retorno limpo,
     *  a memória não pode ser desmapeada no stop — ver nativeStop). */
    bool guestStarted() const { return guestStarted_; }

private:
    uintptr_t memBase_ = 0;   // início da reserva (endereço guest 0)
    uintptr_t guestBase_ = 0; // memBase_ + 0x82000000 (início da imagem)
    XexImageInfo imageInfo_;  // base/entry/tamanho/TLS reais do XEX carregado
    bool guestStarted_ = false;
};

} // namespace fh2::ppc
