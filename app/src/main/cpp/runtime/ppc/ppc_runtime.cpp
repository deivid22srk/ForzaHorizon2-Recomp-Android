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

#include "runtime/audio/audio_output.h"
#include "runtime/fs/fs_provider.h"
#include "runtime/gfx/graphics_backend.h"
#include "runtime/input/input_state.h"
#include "runtime/ppc/kernel_state.h"

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
static constexpr size_t kMemTotal =
    static_cast<size_t>(kMainStackTop + 0x1000000ull);

PpcRuntime::~PpcRuntime() {
    if (memBase_) {
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

        // TABELA MÁGICA de funções: sem ela, QUALQUER chamada indireta
        // (vtable, callback, import) leria ponteiro zero → SIGSEGV imediato.
        const uint64_t populated = populateMagicTable(
            reinterpret_cast<uint8_t*>(memBase_));

        // Heap guest + janela virtual do kernel HLE
        kern::heap().init(kHeapBase, kHeapSize);
        kern::virtWindow().init(kVirtBase, kVirtSize);
        PLOG("guest mapeado: entry 0x%08X — tabela mágica %llu funções "
             "(0x%08llX..0x%08llX), heap 0x%08llX (%llu MB), janela virtual "
             "0x%08llX (%llu MB)",
             info.entryPoint, (unsigned long long)populated,
             (unsigned long long)kImageEnd,
             (unsigned long long)kMagicTableEnd,
             (unsigned long long)kHeapBase, (unsigned long long)(kHeapSize >> 20),
             (unsigned long long)kVirtBase,
             (unsigned long long)(kVirtSize >> 20));
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
        runGuestMain(imageInfo_.entryPoint,
                     reinterpret_cast<uint8_t*>(memBase_), imageInfo_,
                     stopRequested);
        PLOG("run: guest concluído");
        return;
    }
    PLOG("run: sem imagem/entry (modo shell) — aguardando stop");
#else
    PLOG("run: FH2_HAS_RECOMP=0 — modo shell");
#endif

    // Sem guest: mantém o frame loop do app vivo (setup/shell).
    int targetFps = gfx ? gfx->fpsTarget() : 60;
    auto frameInterval =
        std::chrono::milliseconds(1000 / (targetFps > 0 ? targetFps : 60));
    while (!stopRequested) {
        std::this_thread::sleep_for(frameInterval);
    }
    PLOG("run: encerrado");
}

} // namespace fh2::ppc
