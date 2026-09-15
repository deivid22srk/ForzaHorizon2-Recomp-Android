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

namespace {

inline uint32_t be32(const uint8_t* p) {
    return (uint32_t(p[0]) << 24) | (uint32_t(p[1]) << 16) |
           (uint32_t(p[2]) << 8) | uint32_t(p[3]);
}

/** Percorre os cabeçalhos opcionais do XEX2 (área NÃO encriptada) e extrai
 *  TLS / stack / heap / title id — os metadados de que o runtime precisa
 *  para preparar threads guest de verdade. */
void parseOptionalHeaders(const uint8_t* data, size_t size, XexImageInfo& out) {
    if (size < 0x18) return;
    const uint32_t headerCount = be32(data + 0x14);
    const uint8_t* opt = data + 0x18;
    const size_t avail = size - 0x18;
    size_t off = 0;
    for (uint32_t i = 0; i < headerCount && off + 8 <= avail; ++i) {
        const uint32_t key = be32(opt + off);
        const uint32_t val = be32(opt + off + 4);
        const size_t body = off + 8;
        switch (key) {
        case 0x00020104: { // XEX_HEADER_TLS_INFO → val = offset do corpo
            if (val + 32 <= size) {
                const uint8_t* t = data + val;
                out.tlsNumberOfSlots = be32(t);
                out.tlsSlotSize = be32(t + 4);
                out.tlsBytes = be32(t + 8);
                out.tlsDataStart = be32(t + 12);
                out.tlsRawDataEnd = be32(t + 16);
                out.tlsDataEnd = be32(t + 20);
                out.tlsIndexAddr = be32(t + 24);
                out.tlsBaseAddr = be32(t + 28);
            }
            break;
        }
        case 0x00020200: // XEX_HEADER_DEFAULT_STACK_SIZE (valor inline)
            out.defaultStackSize = val;
            break;
        case 0x00020401: // XEX_HEADER_DEFAULT_HEAP_SIZE (valor inline)
            out.defaultHeapSize = val;
            break;
        case 0x00040006: { // XEX_HEADER_EXECUTION_INFO → offset do corpo
            if (val + 0x14 <= size) {
                // MediaID[0x10] e então TitleID (BE)
                out.titleId = be32(data + val + 0x10);
            }
            break;
        }
        default:
            break;
        }
        // tamanho do corpo: chaves ≥ 0x0000FF00 carregam length no 1º u32
        if (key >= 0x0000FF00) {
            if (body + 4 > avail) break;
            const uint32_t len = be32(opt + body);
            off = body + 4 + len;
        } else {
            off = body;
        }
    }
}

} // namespace

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
    parseOptionalHeaders(fileData.data(), fileData.size(), out);
    __android_log_print(ANDROID_LOG_INFO, "FH2/XEX",
                        "default.xex decodificado: base=0x%08X entry=0x%08X "
                        "size=%u bytes titleId=0x%08X stack=%u heap=%u "
                        "TLS=%u bytes (slots=%u x %u, data=0x%08X..0x%08X)",
                        out.base, out.entryPoint, out.imageSize, out.titleId,
                        out.defaultStackSize, out.defaultHeapSize,
                        out.tlsBytes, out.tlsNumberOfSlots, out.tlsSlotSize,
                        out.tlsDataStart, out.tlsDataEnd);
    return true;
}

} // namespace fh2::ppc
