// xex_loader.cpp — decodificação e mapeamento do XEX2 na memória guest
//
// Fluxo real (sem atalhos):
//   1. Lê default.xex do FsProvider (fd SAF direto → pread; fallbacks já
//      implementados na FsProvider).
//   2. Valida o cabeçalho XEX2 (magic + tamanho mínimo) antes de usar
//      estruturas — o Xex2LoadImage do XenonUtils assume cabeçalho íntegro.
//   3. Xex2LoadImage: decripta AES-128 (chave retail + IV nulo), descomprime
//      (none/basic/normal-LZX via libmspack), mapeia seções PE e aplica o
//      patch de imports (nop;nop;nop;blr) — o mesmo estado que o
//      XenonAnalyse/XenonRecomp analisaram, garantindo consistência com o
//      código gerado.
//   4. Validações de sanidade da imagem decodificada (MZ, base, entry,
//      limites da reserva) e cópia para guestMem + base.
#include "xex_loader.h"

#include <android/log.h>
#include <cstring>
#include <vector>

#include "runtime/fs/fs_provider.h"
#include "image.h"
#include "xex.h"

namespace fh2::ppc {

bool loadXexImage(fs::FsProvider& fs, uint8_t* guestMem, size_t guestMemBytes,
                  XexImageInfo& out, std::string& error) {
    out = {};
    error.clear();
    if (!guestMem || guestMemBytes == 0) {
        error = "memória guest não reservada";
        return false;
    }

    std::vector<uint8_t> fileData;
    if (!fs.readFile("default.xex", fileData)) {
        error = "default.xex não pôde ser lido do storage do jogo";
        return false;
    }
    if (fileData.size() < sizeof(Xex2Header) + sizeof(Xex2SecurityInfo)) {
        error = "default.xex menor que o cabeçalho XEX2 (arquivo truncado?)";
        return false;
    }

    const auto* header = reinterpret_cast<const Xex2Header*>(fileData.data());
    if (header->magic != 0x58455832u) { // 'XEX2' em big-endian
        error = "magic XEX2 inválido — o arquivo não é um executável Xbox 360";
        return false;
    }

    Image image = Xex2LoadImage(fileData.data(), fileData.size());
    if (image.data == nullptr || image.size == 0) {
        error = "decodificação XEX falhou (decriptação/descompressão — arquivo "
                "corrompido ou variante não suportada)";
        return false;
    }
    if (image.size < 2 || image.data[0] != 'M' || image.data[1] != 'Z') {
        error = "imagem decodificada sem magic PE ('MZ') — conteúdo inválido";
        return false;
    }
    if (image.base == 0 || image.entry_point == 0) {
        error = "imagem sem base/entry point nos headers opcionais do XEX";
        return false;
    }
    const uint64_t imageEnd = uint64_t(image.base) + image.size;
    if (imageEnd > uint64_t(guestMemBytes)) {
        error = "imagem maior que a memória guest reservada";
        return false;
    }

    // Cópia para o espaço guest: guestMem + base é exatamente o endereço que
    // o código gerado usa (base + 0x82000000 …).
    memcpy(guestMem + image.base, image.data.get(), image.size);

    out.base = static_cast<uint32_t>(image.base);
    out.entryPoint = static_cast<uint32_t>(image.entry_point);
    out.imageSize = image.size;
    __android_log_print(ANDROID_LOG_INFO, "FH2/XEX",
                        "default.xex decodificado: base=0x%08X entry=0x%08X size=%u bytes",
                        out.base, out.entryPoint, out.imageSize);
    return true;
}

} // namespace fh2::ppc
