// ppc_runtime.cpp — bootstrap do guest
//
// Modelo de memória (crítico): o código gerado pelo XenonRecomp acessa a
// memória guest de forma relativa — `PPC_LOAD_U32(x) = *(base + x)` onde `base`
// é recebido POR PARÂMETRO em toda função e x é o endereço guest (ex.:
// 0x82000000 + offset). Logo, qualquer região contígua de kMemTotal bytes é
// válida: reservamos com hint 0x02000000 (endereços baixos facilitam
// depuração) e ACEITAMOS o endereço que o kernel devolver — alguns kernels
// Android não honram hints baixas (região ocupada) e isso NÃO impede o modelo
// base+addr. MAP_NORESERVE: commit só no uso real.
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

#if FH2_HAS_RECOMP
#include "ppc_config.h"
#endif

#define PLOG(...) __android_log_print(ANDROID_LOG_INFO, "FH2/PPC", __VA_ARGS__)

namespace fh2::ppc {

// Endereço virtual desejado: guest base (VA 0 do guest) ficaria aqui.
static constexpr uintptr_t kMemBaseHint = 0x02000000ull;
// Espaço acima do fim da imagem: heap guest + magic function table
// (endereços de função vivem após a região válida do XEX — ver ppc_config.h:
// PPC_CODE_BASE).
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
        // Caminho observado no moto g34 5G (Android 15): o kernel ignora a
        // hint e devolve outra região. O modelo base+addr continua correto —
        // toda função gerada recebe `base` por parâmetro.
        PLOG("memória guest: %zu MB em base dinâmica 0x%016llX (hint 0x%08llX "
             "não honrada — irrelevante: acessos são base+addr) — imagem em "
             "0x%016llX",
             kMemTotal >> 20, (unsigned long long)memBase_,
             (unsigned long long)kMemBaseHint, (unsigned long long)guestBase_);
    }

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
        PLOG("imagem guest mapeada em 0x%016llX (base guest 0x%08X, entry "
             "0x%08X, %u bytes) — execução aguarda kernel/IO (issue #16)",
             (unsigned long long)(memBase_ + info.base), info.base,
             info.entryPoint, info.imageSize);
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
        // TODO(backlog #16): chamada do guest main (entry via ppc_func_mapping)
        // após a camada kernel/IO responder aos imports.
#endif
    }

    PLOG("run: encerrado");
}

} // namespace fh2::ppc
