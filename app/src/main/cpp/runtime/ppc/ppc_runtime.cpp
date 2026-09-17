// ppc_runtime.cpp — bootstrap e execução REAL do guest
//
// Modelo de memória (crítico): o código gerado acessa a memória guest de
// forma relativa — `PPC_LOAD_U32(x) = *(base + x)` onde `base` é recebido
// POR PARÂMETRO em toda função e x é o endereço guest. Logo, qualquer
// região contígua de kMemTotal bytes é válida (hint 0x02000000 é só
// conveniência; MAP_NORESERVE: commit só no uso real).
//
// Mapa do espaço guest (ver DECISIONS D24):
//   0x82000000 + imagem               — XEX decodificado
//   imagem + PPC_CODE_SIZE*2          — TABELA MÁGICA de funções
//       (PPC_LOOKUP_FUNC: base + IMAGE_BASE + IMAGE_SIZE + (addr-CODE_BASE)*2)
//   0x86000000 .. +96 MB              — heap guest (ExAllocatePool)
//   0x8C000000 .. +192 MB             — janela virtual (NtAllocateVirtualMemory,
//                                        MmAllocatePhysicalMemoryEx, stacks)
//   topo .. +16 MB                    — stack da thread principal
#include "ppc_runtime.h"

#include <android/log.h>
#include <cerrno>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <thread>
#include <vector>

#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#if defined(__ANDROID__)
#include <sys/syscall.h>
#endif

#include "runtime/audio/audio_output.h"
#include "runtime/fs/fs_provider.h"
#include "runtime/gfx/graphics_backend.h"
#include "runtime/gpu/cmd_processor.h"
#include "runtime/input/input_state.h"
#include "runtime/ppc/kernel_state.h"
#include "runtime/ppc/kernel_real.h"

#if FH2_HAS_RECOMP
#include "ppc_config.h"
#include "runtime/ppc/guest_entry.h"
#endif

#define PLOG(...) __android_log_print(ANDROID_LOG_INFO, "FH2/PPC", __VA_ARGS__)

namespace fh2::ppc {

// Endereço virtual desejado: guest base (VA 0 do guest) ficaria aqui.
static constexpr uintptr_t kMemBaseHint = 0x02000000ull;

static constexpr size_t kMemImageSize =
#if FH2_HAS_RECOMP
    static_cast<size_t>(PPC_IMAGE_SIZE);
#else
    0x1700000;
#endif

// Tabela mágica: 1 ponteiro por 2 bytes de código gerado (PPC_LOOKUP_FUNC)
static constexpr uint64_t kMagicTableSize =
#if FH2_HAS_RECOMP
    (PPC_CODE_SIZE * 2 + 0xFFFFFull) & ~0xFFFFFull;
#else
    0x2000000ull;
#endif

static constexpr uint64_t kImageEnd = 0x82000000ull + kMemImageSize;
static constexpr uint64_t kMagicTableEnd = kImageEnd + kMagicTableSize;
static constexpr uint64_t kHeapBase =
    (kMagicTableEnd + 0xFFFFFFull) & ~0xFFFFFFull; // alinhado 16 MB
static constexpr uint64_t kHeapSize = 0x06000000ull;   // 96 MB
static constexpr uint64_t kVirtBase = kHeapBase + kHeapSize;
static constexpr uint64_t kVirtSize = 0x0C000000ull;   // 192 MB
static constexpr uint64_t kMainStackTop = kVirtBase + kVirtSize + 0x01000000ull;

// Modelo de memória do Xbox 360 (real):
//   0x80000000..0xA0000000 — RAM flat do título (512 MB)
//   0xA0000000..0xC0000000 — ALIAS FÍSICO: PA n espelhado em 0xA0000000+n.
// As duas janelas mapeiam AS MESMAS páginas (memfd mapeado duas vezes), como
// no console — pools físicos do jogo (ex.: 0xAF000000 = PA 0x0F000000)
// acessam os mesmos bytes da vista flat.
static constexpr uint64_t kRamBase = 0x80000000ull;
static constexpr uint64_t kRamSize = 0x20000000ull;       // 512 MB
static constexpr uint64_t kPhysAliasBase = 0xA0000000ull;
static constexpr uint64_t kPhysAliasSize = 0x20000000ull; // 512 MB
// Alocador físico do kernel: subfaixa do alias livre dos reservados
// (imagem/tabela/heap/janela virtual terminam em PA 0x18000000).
static constexpr uint64_t kPhysAllocBase = 0xB8000000ull; // PA 0x18000000
static constexpr uint64_t kPhysAllocSize = 0x07E00000ull; // 126 MB

static constexpr size_t kMemTotal =
    static_cast<size_t>(kPhysAliasBase + kPhysAliasSize + 0x1000000ull);

PpcRuntime::~PpcRuntime() {
    // Para o processador de comandos do Xenos antes de tocar na memória.
    fh2::gpu::CommandProcessor::instance().stop();
    // fecha arquivos guest abertos (fds de SAF/local) antes de desmapear
    kern::filesCloseAll();
    if (memBase_) {
        // Threads guest podem ainda estar executando (loops que não passam
        // por waits bloqueantes não conseguem fazer unwind via stop).
        // Desmapear a memória sob uma thread viva = SIGSEGV garantido
        // (faults pós-run no log host 16_09: 0x8f5010ac, 0x91507ed8).
        kern::requestStop();
        const size_t alive = kern::joinAll(5000);
        if (alive > 0) {
            // Processo encerrando: manter o mapeamento é seguro (o kernel
            // do host o recolhe no exit) e evita crash das threads remanescentes.
            PLOG("~PpcRuntime: %zu thread(s) guest ainda viva(s) — memória "
                 "guest mantida mapeada (abandono deliberado)", alive);
            memBase_ = 0;
            return;
        }
        munmap(reinterpret_cast<void*>(memBase_), kMemTotal);
        memBase_ = 0;
    }
}

bool PpcRuntime::initialize(fs::FsProvider* fs) {
#if FH2_HAS_RECOMP
    if (memBase_) return true; // idempotente

    // Tentativa 1: hint fixa (sem MAP_FIXED — o kernel pode mover, e tudo bem).
    void* mapped = mmap(reinterpret_cast<void*>(kMemBaseHint), kMemTotal,
                        PROT_READ | PROT_WRITE,
                        MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);
    if (mapped == MAP_FAILED) {
        // Tentativa 2: sem hint — deixa o kernel escolher (VA 64-bit sobra).
        PLOG("mmap com hint falhou (errno=%d) — tentando sem hint", errno);
        mapped = mmap(nullptr, kMemTotal, PROT_READ | PROT_WRITE,
                      MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);
        if (mapped == MAP_FAILED) {
            PLOG("mmap de %zu MB falhou mesmo sem hint (errno=%d) — memória "
                 "guest indisponível neste device", kMemTotal >> 20, errno);
            return false;
        }
    }
    memBase_ = reinterpret_cast<uintptr_t>(mapped);
    guestBase_ = memBase_ + 0x82000000ull;

    // ALIAS FÍSICO REAL: memfd de 512 MB mapeado nas duas janelas
    // (flat 0x80000000 e físico 0xA0000000) — as mesmas páginas RAM, como no
    // console. Sem memfd, segue sem alias (acessos físicos usam páginas
    // anônimas próprias — funcional, porém sem espelho; logado).
    int ramFd = -1;
#if defined(__ANDROID__)
    ramFd = (int)syscall(SYS_memfd_create, "fh2ram", 0);
#else
    ramFd = memfd_create("fh2ram", 0);
#endif
    if (ramFd >= 0 && ftruncate(ramFd, (off_t)kRamSize) != 0) {
        close(ramFd);
        ramFd = -1;
    }
    if (ramFd >= 0) {
        void* flat = mmap(reinterpret_cast<void*>(memBase_ + kRamBase),
                          kRamSize, PROT_READ | PROT_WRITE,
                          MAP_SHARED | MAP_FIXED | MAP_NORESERVE, ramFd, 0);
        void* alias = mmap(reinterpret_cast<void*>(memBase_ + kPhysAliasBase),
                           kPhysAliasSize, PROT_READ | PROT_WRITE,
                           MAP_SHARED | MAP_FIXED | MAP_NORESERVE, ramFd, 0);
        close(ramFd); // os mapeamentos mantêm as páginas vivas
        if (flat == MAP_FAILED || alias == MAP_FAILED) {
            if (flat != MAP_FAILED) munmap(flat, kRamSize);
            if (alias != MAP_FAILED) munmap(alias, kPhysAliasSize);
            ramFd = -1;
        }
    }
    if (ramFd < 0) {
        // fallback: janelas anônimas distintas (sem espelho flat↔físico)
        mmap(reinterpret_cast<void*>(memBase_ + kRamBase), kRamSize,
             PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED |
                                         MAP_NORESERVE, -1, 0);
        mmap(reinterpret_cast<void*>(memBase_ + kPhysAliasBase), kPhysAliasSize,
             PROT_READ | PROT_WRITE,
             MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED | MAP_NORESERVE, -1, 0);
        PLOG("alias físico flat↔físico indisponível (memfd falhou) — janelas "
             "sem espelho (errno=%d)", errno);
    }

    if (memBase_ == kMemBaseHint) {
        PLOG("memória guest: %zu MB (hint fixa) — imagem guest em 0x%08llX",
             kMemTotal >> 20, (unsigned long long)guestBase_);
    } else {
        PLOG("memória guest: %zu MB em base dinâmica 0x%016llX (hint 0x%08llX "
             "não honrada — irrelevante: acessos são base+addr) — imagem em "
             "0x%016llX",
             kMemTotal >> 20, (unsigned long long)memBase_,
             (unsigned long long)kMemBaseHint, (unsigned long long)guestBase_);
    }
    PLOG("modelo de memória: flat 0x%08llX..0x%08llX, alias físico "
         "0x%08llX..0x%08llX (%s)",
         (unsigned long long)kRamBase,
         (unsigned long long)(kRamBase + kRamSize),
         (unsigned long long)kPhysAliasBase,
         (unsigned long long)(kPhysAliasBase + kPhysAliasSize),
         ramFd >= 0 ? "memfd espelhado" : "anônimo sem espelho");

    kern::setGuestMemory(reinterpret_cast<uint8_t*>(memBase_), kMemTotal);

    // Carregamento REAL da imagem guest: lê default.xex do storage (fd SAF
    // direto), decodifica (AES-128 + descompressão + patch de imports
    // idêntico ao da análise) e copia para memBase_ + PPC_IMAGE_BASE.
    if (fs) {
        std::vector<uint8_t> probe;
        if (!fs->readFile("default.xex", probe, /*maxBytes=*/1)) {
            PLOG("default.xex ausente na pasta selecionada — app segue em modo "
                 "shell (coloque o arquivo na pasta e reinicie o jogo)");
            return true;
        }
        XexImageInfo info;
        std::string err;
        if (!loadXexImage(*fs, reinterpret_cast<uint8_t*>(memBase_), kMemTotal,
                          info, err)) {
            PLOG("falha ao carregar default.xex: %s", err.c_str());
            return false;
        }
        imageInfo_ = info;
        kern::setTitleId(info.titleId);
        // Base + privilégios do módulo para XexGetModuleHandle/Section/
        // CheckExecutablePrivilege (semântica real do kernel).
        kern::setImageInfo(info.base, info.privileges);
        // FsProvider para o file I/O do kernel HLE (NtCreateFile/Read — #19)
        kern::setFsBridge(fs);
        // Recursos nomeados do XEX p/ XexGetModuleSection (semântica real)
        if (!info.resources.empty()) {
            std::vector<kern::KernelResource> res;
            for (const auto& r : info.resources) {
                kern::KernelResource kr;
                memcpy(kr.id, r.id, 9);
                kr.va = r.va;
                kr.size = r.size;
                res.push_back(kr);
            }
            kern::setImageResources(res.data(), res.size());
        }

        // TABELA MÁGICA de funções: sem ela, QUALQUER chamada indireta
        // (vtable, callback, import) leria ponteiro zero → SIGSEGV imediato.
        const uint64_t populated = populateMagicTable(
            reinterpret_cast<uint8_t*>(memBase_));

        // Heap guest + janela virtual + janela física do kernel HLE
        kern::heap().init(kHeapBase, kHeapSize);
        kern::virtWindow().init(kVirtBase, kVirtSize);
        kern::physWindow().init(kPhysAllocBase, kPhysAllocSize);

        // MÓDULOS XEX: executável do título + bibliotecas de import
        // (xam.xex/xboxkrnl.exe — ordinais capturados na decodificação,
        // exports = VA dos stubs reais p/ XexGetProcedureAddress).
        for (const auto& lib : info.importLibraries) {
            const uint32_t h = kern::registerModule(lib.name, 0, lib.exports);
            PLOG("módulo registrado: %s (handle 0x%08X, %zu exports)",
                 lib.name.c_str(), h, lib.exports.size());
        }
        kern::registerModule("default.xex", info.base, {});
        // Tabelas completas: o título resolve via XexGetProcedureAddress
        // ordinais que NÃO importa (XInputdFF*, XamParty*, FileTimeToSystemTime)
        // — no console TODOS resolvem; sem isso o boot polling trava.
        kern::registerFullExportTables();

        // Cópia PRISTINA da imagem p/ restauração no relaunch do título.
        pristineImage_.resize(info.imageSize);
        memcpy(pristineImage_.data(),
               reinterpret_cast<uint8_t*>(memBase_) + info.base,
               info.imageSize);
        PLOG("guest mapeado: entry 0x%08X — tabela mágica %llu funções "
             "(0x%08llX..0x%08llX), heap 0x%08llX (%llu MB), janela virtual "
             "0x%08llX (%llu MB), janela física 0x%08llX (%llu MB)",
             info.entryPoint, (unsigned long long)populated,
             (unsigned long long)kImageEnd,
             (unsigned long long)kMagicTableEnd,
             (unsigned long long)kHeapBase, (unsigned long long)(kHeapSize >> 20),
             (unsigned long long)kVirtBase,
             (unsigned long long)(kVirtSize >> 20),
             (unsigned long long)kPhysAllocBase,
             (unsigned long long)(kPhysAllocSize >> 20));
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
    PLOG("run: subsistemas gfx=%s audio=%s input=%s fs=%s",
         gfx ? gfx->name() : "-", audio ? "aaudio" : "-", input ? "ok" : "-",
         fs ? "ok" : "-");
    (void)paused;

    if (audio) audio->start();

#if FH2_HAS_RECOMP
    if (memBase_ && imageInfo_.entryPoint != 0) {
        kern::setInputBridge(input);
        kern::setThreadBody([this](kern::GuestThread& t) {
            guestThreadBody(t, reinterpret_cast<uint8_t*>(memBase_),
                            imageInfo_);
        });
        guestStarted_ = true; // permanente: memória não pode ser desmapeada
        // Processador de comandos do Xenos (issue #17): consome o ring buffer
        // REAL do guest (PM4) e apresenta os frames que o título produzir —
        // ocioso até VdInitializeRingBuffer + CP_RB_WPTR.
        fh2::gpu::CommandProcessor::instance().start();
        // Driver de frames ANTES do primeiro VdSwap: mantém o pipeline
        // gráfico vivo (clear preto — sem conteúdo inventado) enquanto o
        // guest boota. Encerra no PRIMEIRO VdSwap do jogo OU quando o guest
        // termina (loop de relaunch esgotado/erro de boot) — apresentar
        // frames com o título morto fingiria que o jogo está rodando.
        std::atomic<bool> guestActive{true};
        std::thread presentDriver;
        if (gfx) {
            presentDriver = std::thread([gfx, &stopRequested, &guestActive]() {
                const int fps = gfx->fpsTarget() > 0 ? gfx->fpsTarget() : 60;
                const auto interval = std::chrono::milliseconds(1000 / fps);
                PLOG("driver de frames ativo (%s @%d fps) até o primeiro "
                     "VdSwap do guest (surface preta até lá — sem conteúdo "
                     "inventado)",
                     gfx->name(), fps);
                while (!stopRequested && guestActive &&
                       kern::vdGraphics().swapCount == 0) {
                    std::this_thread::sleep_for(interval);
                    gfx->present();
                }
                PLOG("driver de frames encerrado (%llu flip(s) do guest)",
                     (unsigned long long)kern::vdGraphics().swapCount);
            });
        }
        // Loop de RELAUNCH (semântica real do XAM): XamLoaderLaunchTitle/
        // TerminateTitle encerram o título; se houver relaunch pendente o
        // guest re-executa do entry com estado zerado e o launch data
        // preservado (boot 2 do launcher do FH2).
        constexpr int kMaxBoots = 8; // guarda p/ loop de relaunch infinito
        for (int boot = 1;; ++boot) {
            if (boot > 1) {
                PLOG("boot %d do título (relaunch XAM — launch data "
                     "preservado)", boot);
            }
            if (boot > kMaxBoots) {
                PLOG("limite de %d relaunches do título atingido — o jogo "
                     "continua relançando a si mesmo; encerrando o guest p/ "
                     "evitar loop infinito", kMaxBoots);
                // Diagnóstico REAL do loop: quase sempre é o launcher do FH2
                // relançando por arquivo ausente (update:\media.zip etc.).
                // Aviso VISÍVEL (Toast) — falha de boot não pode ser uma tela
                // preta silenciosa.
                if (fs && !fs->lastFailure().empty()) {
                    PLOG("causa provável: leitura de '%s' falhou — verifique "
                         "se a pasta do jogo contém TODOS os arquivos do "
                         "disco (media.zip fica na RAIZ, junto do "
                         "default.xex)",
                         fs->lastFailure().c_str());
                    fs->showBootMessage(
                        "Arquivo do jogo não encontrado: " +
                        fs->lastFailure() +
                        " — verifique se a pasta contém todos os arquivos do "
                        "disco (media.zip na raiz, junto do default.xex)");
                }
                break;
            }
            runGuestMain(imageInfo_.entryPoint,
                         reinterpret_cast<uint8_t*>(memBase_), imageInfo_,
                         stopRequested);
            std::string relaunchPath;
            uint32_t relaunchFlags = 0;
            if (stopRequested ||
                !kern::consumeTitleRelaunch(relaunchPath, relaunchFlags)) {
                break;
            }
            // Diagnóstico do relaunch: as últimas chamadas HLE mostram o que
            // o launcher tentou antes de desistir (arquivo ausente, export
            // não resolvido, mount de cache falho — a causa fica visível no
            // logcat sem precisar de recompile com trace completo).
            kern::traceDump("XamLoaderLaunchTitle — último relaunch");
            // Reset REAL do estado do título (threads já finalizadas pelo
            // runGuestMain; heap/janelas/waitables/TLS zerados) + imagem
            // restaurada ao estado de boot frio.
            kern::resetTitleState();
            fh2::gpu::CommandProcessor::instance().resetForRelaunch();
            if (!pristineImage_.empty()) {
                memcpy(reinterpret_cast<uint8_t*>(memBase_) + imageInfo_.base,
                       pristineImage_.data(), pristineImage_.size());
            }
            PLOG("relançando título (path=\"%s\" flags=0x%08X)",
                 relaunchPath.c_str(), relaunchFlags);
        }
        guestActive = false; // título acabou: driver de frames NÃO apresenta mais
        if (presentDriver.joinable()) presentDriver.join();
        PLOG("run: guest concluído");
        return;
    }
    PLOG("run: sem imagem/entry (modo shell) — aguardando stop");
#else
    PLOG("run: FH2_HAS_RECOMP=0 — modo shell");
#endif

    // Sem guest: mantém o frame loop do app vivo (setup/shell) apresentando
    // frames reais no backend.
    int targetFps = gfx ? gfx->fpsTarget() : 60;
    auto frameInterval =
        std::chrono::milliseconds(1000 / (targetFps > 0 ? targetFps : 60));
    while (!stopRequested) {
        std::this_thread::sleep_for(frameInterval);
        if (gfx) gfx->present();
    }
    PLOG("run: encerrado");
}

} // namespace fh2::ppc
