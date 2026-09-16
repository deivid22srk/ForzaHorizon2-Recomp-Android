// cmd_processor.cpp — implementação do processador de comandos do Xenos.
//
// Semântica dos pacotes PM4 conforme o hardware Xenos (documentação do
// protocolo: xenia-project/xenia — command_processor.cc/xenos.h, BSD; a
// execução aqui é nossa, sobre o modelo de memória do recomp).
//
// Endianness: memória guest é big-endian (igual ao console). Os valores dos
// pacotes são lidos com conversão BE→host; a endianness por-operação do GPU
// (campos k8in16/k8in32 em MEM_WRITE/WAIT_REG_MEM/etc.) é aplicada via
// gpuSwap(), como o hardware faz.
#include "cmd_processor.h"

#include <android/log.h>

#include <chrono>
#include <cstring>
#include <vector>

#include "runtime/gfx/graphics_backend.h"
#include "runtime/ppc/kernel_state.h"

#define CPLOG(...) __android_log_print(ANDROID_LOG_INFO, "FH2/GPU", __VA_ARGS__)
#define CPWARN(...) __android_log_print(ANDROID_LOG_WARN, "FH2/GPU", __VA_ARGS__)
#define CPERR(...) __android_log_print(ANDROID_LOG_ERROR, "FH2/GPU", __VA_ARGS__)

namespace fh2::gpu {

namespace {

// Opcodes type-3 REAIS do Xenos (xenia xenos.h, enum Type3Opcode).
enum Type3Opcode : uint32_t {
    PM4_ME_INIT               = 0x48,
    PM4_NOP                   = 0x10,
    PM4_INDIRECT_BUFFER       = 0x3f,
    PM4_INDIRECT_BUFFER_PFD   = 0x37,
    PM4_WAIT_FOR_IDLE         = 0x26,
    PM4_WAIT_REG_MEM          = 0x3c,
    PM4_WAIT_REG_EQ           = 0x52,
    PM4_WAIT_REG_GTE          = 0x53,
    PM4_WAIT_UNTIL_READ       = 0x5c,
    PM4_WAIT_IB_PFD_COMPLETE  = 0x5d,
    PM4_REG_RMW               = 0x21,
    PM4_REG_TO_MEM            = 0x3e,
    PM4_MEM_WRITE             = 0x3d,
    PM4_MEM_WRITE_CNTR        = 0x4f,
    PM4_COND_EXEC             = 0x44,
    PM4_COND_WRITE            = 0x45,
    PM4_EVENT_WRITE           = 0x46,
    PM4_EVENT_WRITE_SHD       = 0x58,
    PM4_EVENT_WRITE_CFL       = 0x59,
    PM4_EVENT_WRITE_EXT       = 0x5a,
    PM4_EVENT_WRITE_ZPD       = 0x5b,
    PM4_DRAW_INDX             = 0x22,
    PM4_DRAW_INDX_2           = 0x36,
    PM4_DRAW_INDX_BIN         = 0x34,
    PM4_DRAW_INDX_2_BIN       = 0x35,
    PM4_VIZ_QUERY             = 0x23,
    PM4_SET_STATE             = 0x25,
    PM4_SET_CONSTANT          = 0x2d,
    PM4_SET_CONSTANT2         = 0x55,
    PM4_SET_SHADER_CONSTANTS  = 0x56,
    PM4_LOAD_ALU_CONSTANT     = 0x2f,
    PM4_IM_LOAD               = 0x27,
    PM4_IM_LOAD_IMMEDIATE     = 0x2b,
    PM4_LOAD_CONSTANT_CONTEXT = 0x2e,
    PM4_INVALIDATE_STATE      = 0x3b,
    PM4_SET_SHADER_BASES      = 0x4A,
    PM4_SET_BIN_BASE_OFFSET   = 0x4B,
    PM4_SET_BIN_MASK          = 0x50,
    PM4_SET_BIN_SELECT        = 0x51,
    PM4_CONTEXT_UPDATE        = 0x5e,
    PM4_INTERRUPT             = 0x54,
    PM4_XE_SWAP               = 0x64,
    PM4_IM_STORE              = 0x2c,
    PM4_SET_BIN_MASK_LO       = 0x60,
    PM4_SET_BIN_MASK_HI       = 0x61,
    PM4_SET_BIN_SELECT_LO     = 0x62,
    PM4_SET_BIN_SELECT_HI     = 0x63,
};

// Registradores do Xenos usados pelo CP (índices reais, xenia register_table).
constexpr uint32_t XE_GPU_REG_SCRATCH_REG0        = 0x0100;
constexpr uint32_t XE_GPU_REG_SCRATCH_UMSK        = 0x01DC;
constexpr uint32_t XE_GPU_REG_SCRATCH_ADDR        = 0x01DD;
constexpr uint32_t XE_GPU_REG_COHER_STATUS_HOST   = 0x0A31;
constexpr uint32_t XE_GPU_REG_PA_SC_VIZ_QUERY_STATUS_0 = 0x0C44;
constexpr uint32_t XE_GPU_REG_PA_SC_VIZ_QUERY_STATUS_1 = 0x0C45;
constexpr uint32_t XE_GPU_REG_VGT_EVENT_INITIATOR = 0x21F9;
constexpr uint32_t XE_GPU_REG_SHADER_CONSTANT_FETCH_00_0 = 0x4800;

// Endianness por-operação do GPU (campo nos parâmetros dos pacotes).
inline uint32_t gpuSwap(uint32_t value, uint32_t endianness) {
    switch (endianness & 0x3) {
        case 0: return value;                                     // kNone
        case 1: return ((value & 0xFFFFu) << 16) | (value >> 16); // k8in16
        case 2: return __builtin_bswap32(value);                  // k8in32
        default: return value;
    }
}

constexpr uint32_t kSwapSignature = 0x53574150u; // fourcc "SWAP" (BE)

// Tiled2D REAL: layout de macro tiles 32x32 + micro tiles 8x16 com bancos/
// pipes do Xenos — algoritmo do AddrLib R800 (documentado em xenia
// texture_address.h Tiled2D/TiledCombine; mesmo cálculo do hardware).
inline int32_t tiled2DOffset(int32_t x, int32_t y, uint32_t pitchAligned,
                             uint32_t bytesPerBlockLog2) {
    const int32_t outerBlocks =
        ((y >> 5) * int32_t(pitchAligned >> 5) + (x >> 5)) << 6;
    const int32_t innerBlocks = (((y >> 1) & 0b111) << 3) | (x & 0b111);
    const int32_t outerInnerBytes = (outerBlocks | innerBlocks)
                                    << bytesPerBlockLog2;
    const uint32_t bank = (uint32_t(y) >> 4) & 0b1;
    const uint32_t pipe = ((uint32_t(x) >> 3) & 0b11) ^
                          (((uint32_t(y) >> 3) & 0b1) << 1);
    const uint32_t yLsb = uint32_t(y) & 1;
    return int32_t((yLsb << 4) | (pipe << 6) | (bank << 11)) |
           (outerInnerBytes & 0b1111) |
           (((outerInnerBytes >> 4) & 0b1) << 5) |
           (((outerInnerBytes >> 5) & 0b111) << 8) |
           (outerInnerBytes >> 8 << 12);
}

// MMIO do CP na página do GPU mapeada na RAM guest (mesma base do console).
// O registrador r = (endereço & 0xFFFF)/4; o guest escreve em
// 0x7FC80000 + r*4.
constexpr uint64_t kMmioBaseVa = 0x7FC80000ull;
constexpr uint32_t kMmioRegWptr = 0x01C5; // CP_RB_WPTR
constexpr uint32_t kMmioRegRptr = 0x0704; // CP_RB_RPTR

} // namespace

// ============================================================ infraestrutura

CommandProcessor& CommandProcessor::instance() {
    static CommandProcessor s;
    return s;
}

uint8_t* CommandProcessor::physHost(uint32_t pa) {
    // PA do console (RAM de 512 MB) → flat guest VA 0x80000000+PA.
    if (pa >= 0x20000000u) return nullptr;
    uint8_t* base = kern::guestMem();
    if (!base) return nullptr;
    return base + 0x80000000ull + pa;
}

uint32_t CommandProcessor::readPhysU32(uint32_t pa) {
    const uint8_t* p = physHost(pa & ~0x3u);
    if (!p) return 0;
    return __builtin_bswap32(*(const uint32_t*)p);
}

void CommandProcessor::writePhysU32(uint32_t pa, uint32_t v) {
    uint8_t* p = physHost(pa & ~0x3u);
    if (!p) return;
    *(uint32_t*)p = __builtin_bswap32(v);
}

uint32_t CommandProcessor::readWptrMmio() {
    uint8_t* base = kern::guestMem();
    if (!base) return 0;
    // A página MMIO é RAM guest normal (a reserva de 3 GB cobre 0x7FC80000);
    // o driver do título grava o wptr lá exatamente como no console.
    const uint64_t va = kMmioBaseVa + kMmioRegWptr * 4;
    return __builtin_bswap32(*(const volatile uint32_t*)(base + va));
}

void CommandProcessor::writeRptrMmio(uint32_t rptrIndex) {
    uint8_t* base = kern::guestMem();
    if (!base) return;
    const uint64_t va = kMmioBaseVa + kMmioRegRptr * 4;
    *(uint32_t*)(base + va) = __builtin_bswap32(rptrIndex);
}

void CommandProcessor::onRingBufferInitialized(uint32_t basePa,
                                               uint32_t sizeLog2) {
    ringBasePa_ = basePa;
    ringSizeBytes_ = sizeLog2 < 24 ? (1u << (sizeLog2 + 3)) : 0;
    rptrIndex_ = 0;
    CPLOG("CP: ring buffer inicializado (base PA=0x%08X, %u bytes) — "
          "semântica real do console (r4 = log2 em quadwords)",
          basePa, ringSizeBytes_);
}

void CommandProcessor::onRingPointerWriteBack(uint32_t rptrPa,
                                              uint32_t blockLog2) {
    rptrWbPa_ = rptrPa;
    rptrUpdateDwords_ = blockLog2 < 20 ? ((1u << blockLog2) >> 2) : 1;
    if (rptrUpdateDwords_ == 0) rptrUpdateDwords_ = 1;
    CPLOG("CP: write-back do rptr em PA=0x%08X (a cada %u dwords lidos)",
          rptrPa, rptrUpdateDwords_);
}

void CommandProcessor::resetForRelaunch() {
    ringBasePa_ = 0;
    ringSizeBytes_ = 0;
    rptrWbPa_ = 0;
    rptrUpdateDwords_ = 0;
    rptrIndex_ = 0;
    binMask_ = 0;
    binSelect_ = 0;
    lastSwapPa_ = lastSwapW_ = lastSwapH_ = 0;
    memset(regs_, 0, sizeof(regs_));
}

void CommandProcessor::start() {
    if (running_.exchange(true)) return;
    thread_ = std::thread([this] { threadMain(); });
    CPLOG("CP: thread do processador de comandos do Xenos ativa");
}

void CommandProcessor::stop() {
    if (!running_.exchange(false)) return;
    if (thread_.joinable()) thread_.join();
    CPLOG("CP: encerrado (pacotes=%llu swaps=%llu draws=%llu desconhecidos=%llu)",
          (unsigned long long)packetsExecuted_.load(),
          (unsigned long long)swapsPresented_.load(),
          (unsigned long long)drawsSeen_.load(),
          (unsigned long long)unknownOpcodes_.load());
}

// ============================================================ loop principal

void CommandProcessor::threadMain() {
    uint64_t lastReportPackets = 0;
    while (running_.load()) {
        // Ocioso até o título configurar o ring (VdInitializeRingBuffer).
        if (ringBasePa_ == 0 || ringSizeBytes_ == 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(4));
            continue;
        }

        const uint32_t wptr = readWptrMmio();
        if (wptr == 0xBAADF00Du || wptr == rptrIndex_) {
            std::this_thread::sleep_for(std::chrono::milliseconds(2));
            continue;
        }

        // Executa tudo entre rptr e wptr (índices de dword, wrap real do ring).
        if (!executePrimaryBuffer(rptrIndex_, wptr)) {
            // Pacote inválido: resincroniza pelo wptr publicado (o hardware
            // também para o CP até o driver atualizar os ponteiros).
            rptrIndex_ = wptr;
            std::this_thread::sleep_for(std::chrono::milliseconds(4));
            continue;
        }
        rptrIndex_ = wptr;
        writeRptrMmio(rptrIndex_);
        if (rptrWbPa_) writePhysU32(rptrWbPa_, rptrIndex_);

        // Telemetria a cada 100k pacotes.
        const uint64_t now = packetsExecuted_.load();
        if (now - lastReportPackets >= 100000) {
            lastReportPackets = now;
            CPLOG("CP: %llu pacotes executados (%llu swaps, %llu draws)",
                  (unsigned long long)now,
                  (unsigned long long)swapsPresented_.load(),
                  (unsigned long long)drawsSeen_.load());
        }
    }
}

bool CommandProcessor::executePrimaryBuffer(uint32_t startIndex,
                                            uint32_t endIndex) {
    if (ringSizeBytes_ == 0 || (ringSizeBytes_ & (ringSizeBytes_ - 1)) != 0) {
        return false; // tamanho tem que ser potência de 2
    }
    const uint32_t dwordsMask = (ringSizeBytes_ >> 2) - 1;
    uint32_t idx = startIndex & dwordsMask;
    const uint32_t end = endIndex & dwordsMask;
    uint32_t sinceWb = 0;

    while (idx != end && running_.load()) {
        const uint32_t packet = readPhysU32(ringBasePa_ + (idx << 2));
        const uint32_t type = packet >> 30;
        packetsExecuted_.fetch_add(1);
        ++sinceWb;

        if (packet == 0) {
            idx = (idx + 1) & dwordsMask;
            continue;
        }

        switch (type) {
            case 0: { // type-0: escrita sequencial de registradores
                const uint32_t count = ((packet >> 16) & 0x3FFF) + 1;
                const uint32_t baseIndex = packet & 0x7FFF;
                const bool oneReg = ((packet >> 15) & 1) != 0;
                for (uint32_t m = 0; m < count; ++m) {
                    const uint32_t v = readPhysU32(
                        ringBasePa_ + (((idx + 1 + m) & dwordsMask) << 2));
                    writeRegister(oneReg ? baseIndex : baseIndex + m, v);
                }
                idx = (idx + 1 + count) & dwordsMask;
                break;
            }
            case 2: // type-2: NOP
                idx = (idx + 1) & dwordsMask;
                break;
            case 3: { // type-3: opcode
                const uint32_t opcode = (packet >> 8) & 0x7F;
                const uint32_t count = ((packet >> 16) & 0x3FFF) + 1;
                const bool predicated = (packet & 1) != 0;
                if (!executeType3(opcode, count,
                                  ringBasePa_ + (((idx + 1) & dwordsMask) << 2),
                                  predicated, true)) {
                    return false;
                }
                idx = (idx + 1 + count) & dwordsMask;
                break;
            }
            default: // type-1 (raro; não suportado neste marco — logado)
                CPWARN("CP: pacote type-1 no ring (0x%08X) — buffer abortado",
                       packet);
                return false;
        }

        if (rptrUpdateDwords_ && sinceWb >= rptrUpdateDwords_) {
            sinceWb = 0;
            writeRptrMmio(idx);
            if (rptrWbPa_) writePhysU32(rptrWbPa_, idx);
        }
    }
    return true;
}

bool CommandProcessor::executeIndirect(uint32_t pa, uint32_t countDwords) {
    // Buffer indireto: memória física LINEAR (sem wrap), count dwords.
    for (uint32_t i = 0; i < countDwords;) {
        const uint32_t packet = readPhysU32(pa + (i << 2));
        if (packet == 0) { ++i; continue; }
        packetsExecuted_.fetch_add(1);
        const uint32_t type = packet >> 30;
        if (type == 2) { ++i; continue; }
        if (type == 0) {
            const uint32_t count = ((packet >> 16) & 0x3FFF) + 1;
            const uint32_t baseIndex = packet & 0x7FFF;
            const bool oneReg = ((packet >> 15) & 1) != 0;
            for (uint32_t m = 0; m < count; ++m) {
                const uint32_t v = readPhysU32(pa + ((i + 1 + m) << 2));
                writeRegister(oneReg ? baseIndex : baseIndex + m, v);
            }
            i += 1 + count;
            continue;
        }
        if (type == 3) {
            const uint32_t opcode = (packet >> 8) & 0x7F;
            const uint32_t count = ((packet >> 16) & 0x3FFF) + 1;
            const bool predicated = (packet & 1) != 0;
            if (count > countDwords - i) return false; // estoura o buffer
            if (!executeType3(opcode, count, pa + ((i + 1) << 2), predicated,
                              false)) {
                return false;
            }
            i += 1 + count;
            continue;
        }
        CPWARN("CP: pacote type-1 em buffer indireto (0x%08X)", packet);
        return false;
    }
    return true;
}

// ============================================================ type-3

bool CommandProcessor::executeType3(uint32_t opcode, uint32_t count,
                                    uint32_t paramsPa, bool predicated,
                                    bool wrapRing) {
    // Pacotes predicated (bin check): executados apenas se passarem no teste
    // (semântica real do CP — bins ativos & máscara).
    if (predicated) {
        if ((binSelect_ & binMask_) == 0) return true;
        if (opcode == PM4_XE_SWAP) return true; // swap predicated nunca é válido
    }

    // Leitor sequencial dos parâmetros. No ring primário os parâmetros dão
    // WRAP (ring circular de verdade); em buffers indiretos, lineares.
    uint32_t p = paramsPa;
    const uint32_t ringEnd = ringBasePa_ + ringSizeBytes_;
    auto next = [&]() -> uint32_t {
        if (wrapRing && ringSizeBytes_ && p >= ringEnd) p -= ringSizeBytes_;
        const uint32_t v = readPhysU32(p);
        p += 4;
        return v;
    };

    switch (opcode) {
        case PM4_NOP:
            return true; // dados ignorados; avanço feito pelo chamador

        case PM4_INTERRUPT: { // interrupção explícita do stream de comandos
            const uint32_t cpuMask = next();
            CPLOG("CP: PM4_INTERRUPT (cpuMask=0x%08X)", cpuMask);
            return true;
        }

        case PM4_INDIRECT_BUFFER:
        case PM4_INDIRECT_BUFFER_PFD: {
            const uint32_t listPtr = next() & 0x1FFFFFFF;
            const uint32_t listLength = next() & 0xFFFFF;
            if (listLength == 0) return true;
            return executeIndirect(listPtr, listLength);
        }

        case PM4_WAIT_REG_MEM: {
            const uint32_t waitInfo = next();
            const uint32_t pollAddr = next();
            const uint32_t ref = next();
            const uint32_t mask = next();
            const uint32_t wait = next();
            const bool isMemory = (waitInfo & 0x10) != 0;
            // Poll REAL (memória ou registrador do banco), como o hardware.
            for (int spins = 0;; ++spins) {
                uint32_t value;
                if (isMemory) {
                    value = gpuSwap(readPhysU32(pollAddr & ~0x3u),
                                    pollAddr & 0x3);
                } else {
                    value = pollAddr < kRegCount ? regs_[pollAddr] : 0;
                }
                bool matched = false;
                switch (waitInfo & 0x7) {
                    case 0x0: matched = false; break;
                    case 0x1: matched = (value & mask) < ref; break;
                    case 0x2: matched = (value & mask) <= ref; break;
                    case 0x3: matched = (value & mask) == ref; break;
                    case 0x4: matched = (value & mask) != ref; break;
                    case 0x5: matched = (value & mask) >= ref; break;
                    case 0x6: matched = (value & mask) > ref; break;
                    default: matched = true; break;
                }
                if (matched) return true;
                if (spins > 2000000) {
                    CPWARN("CP: WAIT_REG_MEM sem sinalizar (addr=0x%08X "
                           "val=0x%08X ref=0x%08X mask=0x%08X) — seguindo p/ "
                           "não travar o CP (hardware: esperaria)",
                           pollAddr, value, ref, mask);
                    return true;
                }
                if (wait >= 0x100) {
                    std::this_thread::sleep_for(std::chrono::microseconds(
                        (wait >> 8) * 100));
                } else {
                    std::this_thread::yield();
                }
            }
        }

        case PM4_REG_RMW: {
            const uint32_t info = next();
            const uint32_t andMask = next();
            const uint32_t orMask = next();
            uint32_t value = regs_[info & 0x1FFF];
            value = ((info >> 31) & 1) ? value & regs_[andMask & 0x1FFF]
                                       : value & andMask;
            value = ((info >> 30) & 1) ? value | regs_[orMask & 0x1FFF]
                                       : value | orMask;
            writeRegister(info & 0x1FFF, value);
            return true;
        }

        case PM4_REG_TO_MEM: {
            const uint32_t regAddr = next();
            const uint32_t memAddr = next();
            const uint32_t value = regAddr < kRegCount ? regs_[regAddr] : 0;
            writePhysU32(memAddr & ~0x3u, gpuSwap(value, memAddr & 0x3));
            return true;
        }

        case PM4_MEM_WRITE: {
            uint32_t addr = next();
            for (uint32_t i = 0; i + 1 < count; ++i) {
                const uint32_t data = next();
                writePhysU32(addr & ~0x3u, gpuSwap(data, addr & 0x3));
                addr += 4;
            }
            return true;
        }

        case PM4_COND_WRITE: {
            const uint32_t waitInfo = next();
            const uint32_t pollAddr = next();
            const uint32_t ref = next();
            const uint32_t mask = next();
            const uint32_t writeAddr = next();
            const uint32_t writeData = next();
            uint32_t value;
            if (waitInfo & 0x10) {
                value = gpuSwap(readPhysU32(pollAddr & ~0x3u), pollAddr & 0x3);
            } else {
                value = pollAddr < kRegCount ? regs_[pollAddr] : 0;
            }
            bool matched = false;
            switch (waitInfo & 0x7) {
                case 0x1: matched = (value & mask) < ref; break;
                case 0x2: matched = (value & mask) <= ref; break;
                case 0x3: matched = (value & mask) == ref; break;
                case 0x4: matched = (value & mask) != ref; break;
                case 0x5: matched = (value & mask) >= ref; break;
                case 0x6: matched = (value & mask) > ref; break;
                default: break;
            }
            if (matched) {
                if (waitInfo & 0x100) {
                    writePhysU32(writeAddr & ~0x3u,
                                 gpuSwap(writeData, writeAddr & 0x3));
                } else {
                    writeRegister(writeAddr, writeData);
                }
            }
            return true;
        }

        case PM4_EVENT_WRITE: {
            const uint32_t initiator = next();
            writeRegister(XE_GPU_REG_VGT_EVENT_INITIATOR, initiator & 0x3F);
            if (count > 1) {
                // Evento com write-back de endereço (dados do evento).
                const uint32_t addr = next();
                const uint32_t data = next();
                writePhysU32(addr & ~0x3u, gpuSwap(data, addr & 0x3));
            }
            return true;
        }

        case PM4_EVENT_WRITE_SHD: {
            const uint32_t initiator = next();
            const uint32_t addr = next();
            const uint32_t value = next();
            writeRegister(XE_GPU_REG_VGT_EVENT_INITIATOR, initiator & 0x3F);
            const uint32_t data = ((initiator >> 31) & 1) ? 0 : value;
            writePhysU32(addr & ~0x3u, gpuSwap(data, addr & 0x3));
            return true;
        }

        case PM4_EVENT_WRITE_EXT: {
            // Screen extent: o driver lê 6 u16 (x0,x1,y0,y1,z0,z1). O console
            // reporta a extensão real afetada pelo draw anterior; sem
            // rasterização ainda, o report é a extensão máxima (8192/8 como
            // no hardware sem clipping — mesmo valor usado como default).
            const uint32_t initiator = next();
            const uint32_t addr = next();
            writeRegister(XE_GPU_REG_VGT_EVENT_INITIATOR, initiator & 0x3F);
            uint8_t* host = physHost(addr & ~0x3u);
            if (host) {
                const uint16_t extents[6] = {0, 1024, 0, 1024, 0, 1};
                for (int i = 0; i < 6; ++i) {
                    *(uint16_t*)(host + i * 2) =
                        __builtin_bswap16(extents[i]); // BE (k8in16)
                }
            }
            return true;
        }

        case PM4_EVENT_WRITE_ZPD: {
            const uint32_t initiator = next();
            writeRegister(XE_GPU_REG_VGT_EVENT_INITIATOR, initiator & 0x3F);
            return true;
        }

        case PM4_DRAW_INDX:
        case PM4_DRAW_INDX_BIN:
        case PM4_DRAW_INDX_2:
        case PM4_DRAW_INDX_2_BIN: {
            // Draw REAL no estado atual do banco (registradores/constantes/
            // shaders já carregados acima). A RASTERIZAÇÃO (translação do
            // ucode dos shaders para Vulkan) é o PRÓXIMO marco da issue #17 —
            // os parâmetros reais são contados/logados; nada é inventado.
            drawsSeen_.fetch_add(1);
            static uint64_t s_lastLog = 0;
            const uint64_t n = drawsSeen_.load();
            if (n - s_lastLog >= 100) {
                s_lastLog = n;
                CPLOG("CP: draw #%llu (opcode 0x%02X, %u dwords de args) — "
                      "rasterização é o próximo marco da issue #17",
                      (unsigned long long)n, opcode, count);
            }
            return true;
        }

        case PM4_VIZ_QUERY: {
            const uint32_t dword0 = next();
            const uint32_t id = dword0 & 0x3F;
            const bool end = (dword0 & 0x100) != 0;
            if (!end) {
                writeRegister(XE_GPU_REG_VGT_EVENT_INITIATOR, 1); // START
            } else {
                writeRegister(XE_GPU_REG_VGT_EVENT_INITIATOR, 2); // END
                if (id < 32) {
                    regs_[XE_GPU_REG_PA_SC_VIZ_QUERY_STATUS_0] |= 1u << id;
                } else {
                    regs_[XE_GPU_REG_PA_SC_VIZ_QUERY_STATUS_1] |= 1u << (id - 32);
                }
            }
            return true;
        }

        case PM4_SET_CONSTANT: {
            const uint32_t offsetType = next();
            uint32_t index = offsetType & 0x7FF;
            const uint32_t type = (offsetType >> 16) & 0xFF;
            switch (type) {
                case 0: index += 0x4000; break; // ALU
                case 1: index += 0x4800; break; // FETCH
                case 2: index += 0x4900; break; // BOOL
                case 3: index += 0x4908; break; // LOOP
                case 4: index += 0x2000; break; // REGISTERS
                default:
                    CPWARN("CP: SET_CONSTANT tipo desconhecido %u", type);
                    return true;
            }
            for (uint32_t nIdx = 0; nIdx + 1 < count; ++nIdx, ++index) {
                writeRegister(index, next());
            }
            return true;
        }

        case PM4_SET_CONSTANT2:
        case PM4_SET_SHADER_CONSTANTS: {
            const uint32_t offsetType = next();
            uint32_t index = offsetType & 0xFFFF;
            for (uint32_t nIdx = 0; nIdx + 1 < count; ++nIdx, ++index) {
                writeRegister(index, next());
            }
            return true;
        }

        case PM4_LOAD_ALU_CONSTANT: {
            const uint32_t address = next() & 0x3FFFFFFF;
            const uint32_t offsetType = next();
            uint32_t index = offsetType & 0x7FF;
            const uint32_t sizeDwords = next() & 0xFFF;
            const uint32_t type = (offsetType >> 16) & 0xFF;
            switch (type) {
                case 0: index += 0x4000; break;
                case 1: index += 0x4800; break;
                case 2: index += 0x4900; break;
                case 3: index += 0x4908; break;
                case 4: index += 0x2000; break;
                default:
                    CPWARN("CP: LOAD_ALU_CONSTANT tipo desconhecido %u", type);
                    return true;
            }
            for (uint32_t nIdx = 0; nIdx < sizeDwords; ++nIdx, ++index) {
                writeRegister(index, readPhysU32(address + nIdx * 4));
            }
            return true;
        }

        case PM4_IM_LOAD: {
            // Upload de shader por PONTEIRO: ucode REAL do título (capturado;
            // a compilação para Vulkan é o próximo marco).
            const uint32_t addrType = next();
            const uint32_t shaderType = addrType & 0x3;
            const uint32_t addr = addrType & ~0x3u;
            const uint32_t startSize = next();
            const uint32_t sizeDwords = startSize & 0xFFFF;
            CPLOG("CP: IM_LOAD shader tipo=%u addr=0x%08X size=%u dwords "
                  "(ucode capturado)", shaderType, addr, sizeDwords);
            return true;
        }

        case PM4_IM_LOAD_IMMEDIATE: {
            const uint32_t dword0 = next();
            const uint32_t dword1 = next();
            const uint32_t shaderType = dword0 & 0x3;
            const uint32_t sizeDwords = dword1 & 0xFFFF;
            CPLOG("CP: IM_LOAD_IMMEDIATE shader tipo=%u size=%u dwords",
                  shaderType, sizeDwords);
            return true; // ucode embutido nos params — avançado pelo chamador
        }

        case PM4_ME_INIT:             // micro-engine do CP: sem bins ainda
        case PM4_INVALIDATE_STATE:
        case PM4_SET_STATE:
        case PM4_WAIT_FOR_IDLE:
        case PM4_CONTEXT_UPDATE:
        case PM4_SET_SHADER_BASES:
        case PM4_SET_BIN_BASE_OFFSET:
        case PM4_WAIT_REG_EQ:
        case PM4_WAIT_REG_GTE:
        case PM4_WAIT_UNTIL_READ:
        case PM4_WAIT_IB_PFD_COMPLETE:
        case PM4_MEM_WRITE_CNTR:
        case PM4_IM_STORE:
        case PM4_LOAD_CONSTANT_CONTEXT:
        case PM4_COND_EXEC: {
            // Consumidos com avanço correto (a contagem vem do header);
            // semântica adicional só se o título depender deles.
            return true;
        }

        case PM4_SET_BIN_MASK: {
            const uint64_t hi = next();
            const uint64_t lo = next();
            binMask_ = (hi << 32) | lo;
            return true;
        }
        case PM4_SET_BIN_SELECT: {
            const uint64_t hi = next();
            const uint64_t lo = next();
            binSelect_ = (hi << 32) | lo;
            return true;
        }
        case PM4_SET_BIN_MASK_LO: {
            binMask_ = (binMask_ & 0xFFFFFFFF00000000ull) | next();
            return true;
        }
        case PM4_SET_BIN_MASK_HI: {
            binMask_ = (binMask_ & 0xFFFFFFFFull) | (uint64_t(next()) << 32);
            return true;
        }
        case PM4_SET_BIN_SELECT_LO: {
            binSelect_ = (binSelect_ & 0xFFFFFFFF00000000ull) | next();
            return true;
        }
        case PM4_SET_BIN_SELECT_HI: {
            binSelect_ = (binSelect_ & 0xFFFFFFFFull) | (uint64_t(next()) << 32);
            return true;
        }

        case PM4_XE_SWAP: {
            const uint32_t magic = next();
            if (magic != kSwapSignature) {
                CPWARN("CP: XE_SWAP com assinatura inválida 0x%08X", magic);
                return true;
            }
            const uint32_t frontBufferPa = next();
            const uint32_t width = next();
            const uint32_t height = next();
            issueSwap(frontBufferPa, width, height);
            return true;
        }

        default:
            unknownOpcodes_.fetch_add(1);
            static uint64_t s_lastUnknownLog = 0;
            const uint64_t un = unknownOpcodes_.load();
            if (un - s_lastUnknownLog >= 16) {
                s_lastUnknownLog = un;
                CPWARN("CP: opcode não implementado 0x%02X (%u dwords) — "
                       "avançado sem efeito (contagem real: %llu)",
                       opcode, count, (unsigned long long)un);
            }
            return true;
    }
}

// ============================================================ registradores

void CommandProcessor::writeRegister(uint32_t index, uint32_t value) {
    if (index >= kRegCount) {
        static uint32_t s_oob = 0;
        if (++s_oob <= 8) {
            CPWARN("CP: escrita fora do banco de registradores 0x%04X", index);
        }
        return;
    }
    regs_[index] = value;

    // Scratch: write-back REAL para memória guest (máscara SCRATCH_UMSK).
    if (index >= XE_GPU_REG_SCRATCH_REG0 &&
        index <= XE_GPU_REG_SCRATCH_REG0 + 7) {
        const uint32_t scratchReg = index - XE_GPU_REG_SCRATCH_REG0;
        if ((1u << scratchReg) & regs_[XE_GPU_REG_SCRATCH_UMSK]) {
            const uint32_t addr =
                regs_[XE_GPU_REG_SCRATCH_ADDR] + scratchReg * 4;
            writePhysU32(addr, value);
        }
    } else if (index == XE_GPU_REG_COHER_STATUS_HOST) {
        regs_[index] |= 0x80000000u;
    }
}

// ============================================================ swap real

void CommandProcessor::issueSwap(uint32_t frontBufferPa, uint32_t width,
                                 uint32_t height) {
    // Formato/pitch/tile REAIS do front buffer: o VdSwap gravou o texture
    // fetch do D3D9 nos registradores FETCH_00_0..5 (pacote type-0
    // imediatamente anterior no ring — mesma sequência do console).
    const uint32_t dword0 = regs_[XE_GPU_REG_SHADER_CONSTANT_FETCH_00_0];
    const uint32_t dword1 = regs_[XE_GPU_REG_SHADER_CONSTANT_FETCH_00_0 + 1];
    const uint32_t xenosFormat = dword1 & 0x3F;         // TextureFormat
    const bool tiled = (dword0 >> 31) & 1;
    const uint32_t pitchField = (dword0 >> 22) & 0x1FF; // pixels >> 5

    lastSwapPa_ = frontBufferPa;
    lastSwapW_ = width;
    lastSwapH_ = height;

    uint8_t* host = physHost(frontBufferPa);
    if (!host) {
        CPERR("CP: swap com front buffer PA inválido 0x%08X", frontBufferPa);
        return;
    }
    if (width == 0 || height == 0 || width > 4096 || height > 4096) {
        CPERR("CP: swap com dimensões inválidas %ux%u (PA 0x%08X)",
              width, height, frontBufferPa);
        return;
    }

    const uint32_t rowPixels = pitchField ? (pitchField << 5) : width;
    const uint32_t rowBytes = rowPixels * 4; // k_8_8_8_8 = 4 bytes/pixel
    bool presented = false;

    if (xenosFormat != 6 /* k_8_8_8_8 */) {
        static uint32_t s_fmtWarn = 0;
        if (++s_fmtWarn <= 4) {
            CPWARN("CP: swap em formato %u (tiled=%d) — suporte no próximo "
                   "marco; front buffer REAL não exibido", xenosFormat, tiled);
        }
    } else if (tiled) {
        // Front buffers D3D9 do 360 são tipicamente TILED — desserialização
        // REAL do layout de tiles p/ linear antes do upload.
        const uint32_t pitchAligned = (rowPixels + 31) & ~31u;
        sLinear_.resize((size_t)rowBytes * height);
        uint8_t* dst = sLinear_.data();
        for (uint32_t y = 0; y < height; ++y) {
            for (uint32_t x = 0; x < width; ++x) {
                const int32_t off =
                    tiled2DOffset((int32_t)x, (int32_t)y, pitchAligned, 2);
                const uint8_t* src = host + off;
                uint8_t* d = dst + (size_t)y * rowBytes + (size_t)x * 4;
                d[0] = src[0]; d[1] = src[1]; d[2] = src[2]; d[3] = src[3];
            }
        }
        if (auto* backend = fh2::gfx::activeBackend()) {
            presented = backend->presentFrontBuffer(dst, rowBytes, width,
                                                    height, xenosFormat);
        }
    } else {
        // Linear 32bpp: bytes na memória guest = R,G,B,A (BE) — upload direto.
        if (auto* backend = fh2::gfx::activeBackend()) {
            presented = backend->presentFrontBuffer(host, rowBytes, width,
                                                    height, xenosFormat);
        }
    }

    if (presented) swapsPresented_.fetch_add(1);
    static uint64_t s_swapLog = 0;
    const uint64_t sn = swapsPresented_.load();
    if (sn != 0 && sn - s_swapLog >= 300) {
        s_swapLog = sn;
        CPLOG("CP: %llu swaps apresentados (front buffer %ux%u fmt=%u)",
              (unsigned long long)sn, width, height, xenosFormat);
    }
}

} // namespace fh2::gpu
