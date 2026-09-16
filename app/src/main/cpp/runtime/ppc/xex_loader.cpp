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
 *  TLS / stack / heap / title id / privilégios — os metadados de que o
 *  runtime precisa para preparar threads guest de verdade.
 *
 *  Walk IDÊNTICO ao do XenonUtils (getOptHeaderPtr): array flat de
 *  {key u32, value u32} a partir de 0x18, headerCount entradas. O tipo é
 *  dado pelo byte baixo da key:
 *    key & 0xFF == 0x00 → value É o dado (u32 inline)
 *    key & 0xFF == 0x01 → value é o dado (u32 inline, ponteiro)
 *    demais (0x04, 0xFF, …) → value é OFFSET no arquivo do corpo */
void parseOptionalHeaders(const uint8_t* data, size_t size, XexImageInfo& out) {
    if (size < 0x18) return;
    const uint32_t headerCount = be32(data + 0x14);
    const uint8_t* opt = data + 0x18;
    for (uint32_t i = 0; i < headerCount; ++i) {
        const size_t off = size_t(i) * 8;
        if (off + 8 > size - 0x18) break;
        const uint32_t key = be32(opt + off);
        const uint32_t val = be32(opt + off + 4);
        const uint8_t* body = nullptr;
        switch (key & 0xFF) {
        case 0x00:
        case 0x01:
            body = opt + off + 4; // inline (aponta para o próprio value)
            break;
        default:
            if (size_t(val) + 32 <= size) body = data + val; // offset no arquivo
            break;
        }
        if (!body) continue;
        switch (key) {
        case 0x000002FF: { // XEX_HEADER_RESOURCE_INFO (corpo: len u32 + N×16)
            // {u32 bytes} {char id[8]; u32 va; u32 size} × N — os recursos
            // que o kernel expõe via XexGetModuleSection.
            if (size_t(val) + 4 <= size) {
                const uint32_t bodyLen = be32(body);
                const int count = bodyLen >= 4 ? (int)((bodyLen - 4) / 16) : 0;
                for (int r = 0; r < count && size_t(val) + 4 + size_t(r + 1) * 16 <= size; ++r) {
                    const uint8_t* e = body + 4 + r * 16;
                    XexImageInfo::Resource res{};
                    for (int c = 0; c < 8; ++c) {
                        const char ch = (char)e[c];
                        res.id[c] = (ch >= 32 && ch < 127) ? ch : 0;
                    }
                    res.id[8] = 0;
                    res.va = be32(e + 8);
                    res.size = be32(e + 12);
                    if (res.id[0] && res.size) out.resources.push_back(res);
                }
            }
            break;
        }
        case 0x00020104: { // XEX_HEADER_TLS_INFO (16 bytes, por offset)
            // slots | VA do template na imagem | size init | size total
            out.tlsNumberOfSlots = be32(body);
            out.tlsBaseAddr = be32(body + 4);
            out.tlsDataSize = be32(body + 8);
            out.tlsTotalSize = be32(body + 12);
            break;
        }
        case 0x00020200: // XEX_HEADER_DEFAULT_STACK_SIZE (inline)
            out.defaultStackSize = val;
            break;
        case 0x00020401: // XEX_HEADER_DEFAULT_HEAP_SIZE (inline)
            out.defaultHeapSize = val;
            break;
        case 0x00040006: { // XEX_HEADER_EXECUTION_INFO (por offset)
            // mediaID[0x10] | titleID u32 | launcher u32 | privileges u64
            if (size_t(val) + 0x20 <= size) {
                out.titleId = be32(data + val + 0x10);
                out.privileges = (uint64_t(be32(data + val + 0x18)) << 32) |
                                 be32(data + val + 0x1C);
            }
            break;
        }
        default:
            break;
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
                        "size=%u bytes titleId=0x%08X privileges=%016llX "
                        "stack=%u heap=%u TLS=%u bytes (slots=%u, template "
                        "0x%08X)",
                        out.base, out.entryPoint, out.imageSize, out.titleId,
                        (unsigned long long)out.privileges,
                        out.defaultStackSize, out.defaultHeapSize,
                        out.tlsTotalSize, out.tlsNumberOfSlots,
                        out.tlsBaseAddr);
    return true;
}

} // namespace fh2::ppc
