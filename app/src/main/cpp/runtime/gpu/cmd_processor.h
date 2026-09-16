// cmd_processor.h — processador de comandos do Xenos (issue #17, marco real)
//
// Emula o CP (Command Processor) do GPU Xenos com a SEMÂNTICA REAL do
// hardware/console (referência: xenia-project/xenia, src/xenia/gpu/
// command_processor.cc — BSD, usada apenas como documentação do protocolo):
//
//   • o título configura o ring buffer físico via VdInitializeRingBuffer
//     (base PA + log2 do tamanho em QUADWORDS) e o write-back do read
//     pointer via VdEnableRingBufferRPtrWriteBack (PA + log2 do bloco);
//   • o driver do título (D3D9) publica comandos PM4 no ring e escreve o
//     write pointer no registrador MMIO CP_RB_WPTR (VA do guest 0x7FC8071C);
//   • o CP consome os pacotes entre rptr e wptr, executa e escreve o rptr
//     de volta (write-back físico + espelho no MMIO CP_RB_RPTR 0x7FC81C10);
//   • VdSwap (kernel) enfileira o pacote PM4_XE_SWAP com o front buffer; o
//     CP o processa e apresenta o frame REAL via backend gráfico.
//
// Estado real mantido: banco de registradores do GPU (constantes ALU/FETCH/
// BOOL/LOOP, registers 0x2000+), shaders carregados (ucode capturado),
// bin mask/select, contadores. A RASTERIZAÇÃO dos draws (translação de
// ucode → Vulkan) é o PRÓXIMO marco — os draws são contados com seus
// parâmetros reais, jamais inventados.
#pragma once

#include <atomic>
#include <cstdint>
#include <thread>
#include <vector>

namespace fh2::gpu {

class CommandProcessor {
public:
    static CommandProcessor& instance();

    // Idempotente: cria a thread do CP. A thread fica ociosa até o título
    // configurar o ring buffer (Vd*) e publicar wptr no MMIO.
    void start();

    // Para a thread (shutdown). Seguro chamar mais de uma vez.
    void stop();

    // Reset do estado de sessão (relaunch do título: novo D3D reconfigura
    // tudo — registradores, rptr, shaders zerados como em boot frio).
    void resetForRelaunch();

    // Chamado pelo kernel (real_VdInitializeRingBuffer) com os valores
    // REAIS que o guest passou: base = PA físico, sizeLog2 = log2 do
    // tamanho em quadwords (1 << (sizeLog2+3) bytes).
    void onRingBufferInitialized(uint32_t basePa, uint32_t sizeLog2);

    // Chamado pelo kernel (real_VdEnableRingBufferRPtrWriteBack): PA do
    // write-back do rptr + log2 do bloco (dwords lidos entre updates).
    void onRingPointerWriteBack(uint32_t rptrPa, uint32_t blockLog2);

    // Contadores reais (diagnóstico/log).
    uint64_t packetsExecuted() const { return packetsExecuted_.load(); }
    uint64_t swapsPresented() const { return swapsPresented_.load(); }
    uint64_t drawsSeen() const { return drawsSeen_.load(); }
    uint64_t unknownOpcodes() const { return unknownOpcodes_.load(); }

private:
    CommandProcessor() = default;
    ~CommandProcessor() { stop(); }

    void threadMain();

    // --- acesso à memória guest física (big-endian, como no console) ---
    // host p/ PA: guestMem() + 0x80000000 + PA (RAM do console < 512 MB).
    static uint8_t* physHost(uint32_t pa);
    static uint32_t readPhysU32(uint32_t pa);
    static void writePhysU32(uint32_t pa, uint32_t v);

    // MMIO do GPU: página mapeada na RAM guest em 0x7FC80000 (o driver do
    // título escreve/lê registradores do CP nela — mesmo layout do console).
    static uint32_t readWptrMmio();
    static void writeRptrMmio(uint32_t rptrIndex);

    // Execução de pacotes PM4 (retorna false se o buffer ficou inválido).
    bool executePrimaryBuffer(uint32_t startIndex, uint32_t endIndex);
    // Executa `count` dwords de pacotes a partir de um PA físico (indirect).
    bool executeIndirect(uint32_t pa, uint32_t countDwords);
    // Executa um pacote type-3 individual (pa do dword de params). wrapRing:
    // params no ring primário dão wrap circular; buffers indiretos, lineares.
    bool executeType3(uint32_t opcode, uint32_t count, uint32_t paramsPa,
                      bool predicated, bool wrapRing);

    // Registradores (banco real do Xenos).
    static constexpr uint32_t kRegCount = 0x6000;
    uint32_t regs_[kRegCount] = {};
    void writeRegister(uint32_t index, uint32_t value);

    // Swap REAL: front buffer do título → backend gráfico.
    void issueSwap(uint32_t frontBufferPa, uint32_t width, uint32_t height);

    std::thread thread_;
    std::atomic<bool> running_{false};

    // Config do ring (valores REAIS do guest).
    uint32_t ringBasePa_ = 0;
    uint32_t ringSizeBytes_ = 0;
    uint32_t rptrWbPa_ = 0;
    uint32_t rptrUpdateDwords_ = 0; // (1 << blockLog2) >> 2
    uint32_t rptrIndex_ = 0;

    // Estado de sessão (bin check do CP — packets predicated).
    uint64_t binMask_ = 0;
    uint64_t binSelect_ = 0;

    // Front buffer ativo (do pacote swap; formato vem do FETCH_00 registrado
    // pelo pacote type-0 que o VdSwap grava no banco).
    uint32_t lastSwapPa_ = 0;
    uint32_t lastSwapW_ = 0;
    uint32_t lastSwapH_ = 0;

    // Scratch p/ desserialização de front buffer tiled (thread do CP).
    std::vector<uint8_t> sLinear_;

    std::atomic<uint64_t> packetsExecuted_{0};
    std::atomic<uint64_t> swapsPresented_{0};
    std::atomic<uint64_t> drawsSeen_{0};
    std::atomic<uint64_t> unknownOpcodes_{0};
};

} // namespace fh2::gpu
