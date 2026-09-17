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
#include <map>
#include <vector>

#include "runtime/fs/fs_provider.h"
#include "runtime/ppc/kernel_state.h"
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

    std::vector<Xex2ImportRecord> importRecords;
    Image image = Xex2LoadImageEx(fileData.data(), fileData.size(),
                                  &importRecords);
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

    // Agrupa os imports por biblioteca (xam.xex, xboxkrnl.exe, ...) —
    // ordinal → stub VA. Alimenta o registro de módulos do kernel
    // (XexLoadImage/XexGetProcedureAddress com semântica real).
    {
        uint32_t captured = 0;
        for (const auto& rec : importRecords) {
            XexImageInfo::ImportLibrary* lib = nullptr;
            for (auto& l : out.importLibraries) {
                if (l.name == rec.library) { lib = &l; break; }
            }
            if (!lib) {
                out.importLibraries.emplace_back();
                lib = &out.importLibraries.back();
                lib->name = rec.library;
            }
            lib->exports[rec.ordinal] = rec.thunkVa;
            ++captured;
        }
        if (captured) {
            __android_log_print(ANDROID_LOG_INFO, "FH2/XEX",
                                "imports capturados: %u exports em %zu "
                                "biblioteca(s)", captured,
                                out.importLibraries.size());
        }
    }
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

// ------------------------------------------------- XEX secundário (runtime)

// Região de módulos carregados em runtime (dentro da RAM flat do título,
// acima das alocações do FH2 — 0x82000000..0x8F500000 já em uso no boot).
// Colocação DETERMINÍSTICA: as exports são calculadas relativas a esta base.
constexpr uint64_t kModuleRegionBase = 0x98000000ull;
constexpr uint64_t kModuleRegionSize = 0x07000000ull; // 112 MB
uint64_t g_moduleRegionNext = kModuleRegionBase;

namespace {

inline uint32_t be32r(const uint8_t* p) {
    return (uint32_t(p[0]) << 24) | (uint32_t(p[1]) << 16) |
           (uint32_t(p[2]) << 8) | uint32_t(p[3]);
}

inline uint16_t be16r(const uint8_t* p) {
    return (uint16_t)((uint16_t(p[0]) << 8) | uint16_t(p[1]));
}

/** Resolve as exports do módulo via PE EXPORT DIRECTORY do XEX decodificado
 *  — a semântica REAL do XexGetProcedureAddress no console: o kernel caminha
 *  o PE do módulo (DOS → NT → OptionalHeader.DataDirectory[0] →
 *  IMAGE_EXPORT_DIRECTORY), NÃO um header opcional do XEX. O XMediaFacade/
 *  SpeechFacade não têm XEX_HEADER_EXPORTS_BY_NAME (0x00E10402), mas TÊM a
 *  tabela de exports PE padrão — é por isso que exports=0 estava errado.
 *
 *  RVAs do PE indexam o buffer decodificado (imagem flat, RVA == offset).
 *  Retorna ordinal (Base + índice) → VA absoluto do módulo carregado. */
std::map<uint32_t, uint32_t> parseExportTable(uint8_t* imageHost,
                                              uint32_t imageSize,
                                              uint32_t moduleVa) {
    std::map<uint32_t, uint32_t> out;
    if (!imageHost || imageSize < 0x40 + 0x28) return out;
    if (imageHost[0] != 'M' || imageHost[1] != 'Z') return out;
    if (imageSize < 0x3C + 4) return out;
    const uint32_t eLfanew = be32r(imageHost + 0x3C);
    if (eLfanew == 0 || eLfanew + 0x18 + 0xE0 > imageSize) return out; // NT headers completos
    const uint8_t* pe = imageHost + eLfanew;
    if (!(pe[0] == 'P' && pe[1] == 'E' && pe[2] == 0 && pe[3] == 0)) return out;
    const uint8_t* opt = pe + 0x18; // COFF header: 20 bytes
    if (be16r(opt) != 0x10B) return out; // PE32 (Xbox 360)
    // DataDirectory[0] = Export Directory (offset 0x60 no optional header)
    const uint32_t ddRva = be32r(opt + 0x60);
    const uint32_t ddSize = be32r(opt + 0x64);
    if (ddRva == 0 || ddSize < 0x28) return out; // módulo sem exports (real)
    if (ddRva + ddSize > imageSize) return out;
    const uint8_t* d = imageHost + ddRva;
    const uint32_t numberOfFunctions = be32r(d + 0x14);
    const uint32_t addrFunctions = be32r(d + 0x1C);
    const uint32_t ordinalBase = be32r(d + 0x10);
    if (numberOfFunctions == 0 || numberOfFunctions > 0x10000) return out;
    if (addrFunctions == 0 || addrFunctions + numberOfFunctions * 4 > imageSize)
        return out;
    const uint8_t* funcTable = imageHost + addrFunctions;
    for (uint32_t i = 0; i < numberOfFunctions; ++i) {
        const uint32_t rva = be32r(funcTable + i * 4);
        if (rva == 0) continue;
        if (rva >= ddRva && rva < ddRva + ddSize) continue; // forwarder (string)
        if (rva >= imageSize) continue;
        // export VA = base preferida do módulo + RVA (o console carrega o
        // módulo na base preferida — ponteiros absolutos do código continuam
        // válidos, e XexGetProcedureAddress devolve exatamente esse VA)
        out[ordinalBase + i] = moduleVa + rva;
    }
    return out;
}

/** Resolve as exports do módulo pela TABELA XEX2 NATIVA (xex2_export_table)
 *  — a estrutura que o kernel do console realmente usa. O VA da tabela vem
 *  do CAMPO export_table do SECURITY HEADER do XEX (offset 0x160, plaintext
 *  no arquivo), e a tabela vive DENTRO da imagem decodificada:
 *    imagebaseaddr (+0x20, <<16) | count (+0x24) | base (+0x28) |
 *    ordOffset[] (+0x2C)   →   VA(i) = ordOffset[i] + (imagebaseaddr << 16)
 *  O XMediaFacade/SpeechFacade exportam assim (14 e N ordinais); o PE .edata
 *  deles é uma área zero no span (RVA 0x2B0000 > conteúdo 0x288000) — por
 *  isso exports=0 estava errado. Os VAs caem na faixa de código RECOMPILADO
 *  do módulo (despachados pela tabela extra — fh2_funcSlot), então a chamada
 *  do título executa código real do facade, nunca um stub. */
std::map<uint32_t, uint32_t> parseXex2ExportTable(
        const uint8_t* fileData, size_t fileSize,
        const uint8_t* imageHost, uint32_t contentBytes) {
    std::map<uint32_t, uint32_t> out;
    if (!fileData || fileSize < 0x98 + 0x164) return out;
    const auto* header = reinterpret_cast<const Xex2Header*>(fileData);
    const auto* security =
        reinterpret_cast<const Xex2SecurityInfo*>(fileData +
                                                  header->securityOffset);
    // export_table e load_address são big-endian nos bytes PLAINTEXT do
    // security header (só os dados após header.size são cifrados)
    const uint32_t exportTableVa = security->exportTable;
    const uint32_t loadAddress = security->loadAddress;
    if (exportTableVa == 0 || exportTableVa < loadAddress) return out;
    const uint64_t off = (uint64_t)exportTableVa - loadAddress;
    if (off + 0x2C > contentBytes) return out;

    auto rdBE = [&imageHost](uint64_t o) {
        return (uint32_t(imageHost[o]) << 24) |
               (uint32_t(imageHost[o + 1]) << 16) |
               (uint32_t(imageHost[o + 2]) << 8) | uint32_t(imageHost[o + 3]);
    };
    const uint32_t imagebaseaddr = rdBE(off + 0x20);
    const uint32_t count = rdBE(off + 0x24);
    const uint32_t base = rdBE(off + 0x28);
    if (count == 0 || count > 0x10000) return out;
    if (off + 0x2C + (uint64_t)count * 4 > contentBytes) return out;
    for (uint32_t i = 0; i < count; ++i) {
        const uint32_t ordOffset = rdBE(off + 0x2C + (uint64_t)i * 4);
        const uint32_t va = ordOffset + (imagebaseaddr << 16);
        if (va == 0) continue;
        out[base + i] = va;
    }
    return out;
}

} // namespace

/** Tamanho REAL do conteúdo decodificável do XEX (o que existe de bytes no
 *  arquivo): BASIC = soma dos blocos {dataSize+zeroSize}; NONE e NORMAL(LZX)
 *  = security->imageSize. O Xex2LoadImage aloca exatamente esse tanto —
 *  security->imageSize pode ser MAIOR (span com zero-fill no console). */
static uint32_t secondaryContentBytes(const uint8_t* fileData,
                                      size_t fileSize) {
    if (fileSize < 0x30) return 0;
    const auto* header = reinterpret_cast<const Xex2Header*>(fileData);
    if (header->securityOffset + 8 > fileSize) return 0;
    const auto* security =
        reinterpret_cast<const Xex2SecurityInfo*>(fileData +
                                                  header->securityOffset);
    const uint32_t declared = security->imageSize;
    const auto* ffi = reinterpret_cast<const Xex2OptFileFormatInfo*>(
        getOptHeaderPtr(fileData, XEX_HEADER_FILE_FORMAT_INFO));
    if (!ffi) return declared;
    if (ffi->compressionType != XEX_COMPRESSION_BASIC) return declared;
    const auto* blocks =
        reinterpret_cast<const Xex2FileBasicCompressionBlock*>(ffi + 1);
    const size_t numBlocks =
        (ffi->infoSize / sizeof(Xex2FileBasicCompressionInfo)) - 1;
    uint64_t total = 0;
    for (size_t i = 0; i < numBlocks; ++i) {
        total += blocks[i].dataSize + blocks[i].zeroSize;
    }
    if (total == 0 || total > 0x10000000ull) return declared;
    return (uint32_t)total;
}

uint32_t loadSecondaryModule(fs::FsProvider& fs, const std::string& relPath,
                             const std::string& displayName) {
    // 1) leitura REAL do arquivo do jogo
    std::vector<uint8_t> fileData;
    if (!fs.readFile(relPath, fileData)) {
        __android_log_print(ANDROID_LOG_WARN, "FH2/XEX",
                            "XEX secundário '%s' não pôde ser lido do "
                            "storage do jogo", displayName.c_str());
        return 0;
    }
    if (fileData.size() < sizeof(Xex2Header) + sizeof(Xex2SecurityInfo)) {
        __android_log_print(ANDROID_LOG_WARN, "FH2/XEX",
                            "XEX secundário '%s' truncado (%zu bytes)",
                            displayName.c_str(), fileData.size());
        return 0;
    }
    const auto* header = reinterpret_cast<const Xex2Header*>(fileData.data());
    if (header->magic != 0x58455832u) {
        __android_log_print(ANDROID_LOG_WARN, "FH2/XEX",
                            "XEX secundário '%s': magic inválido",
                            displayName.c_str());
        return 0;
    }
    // 2) decodificação REAL (mesmo pipeline do default.xex)
    std::vector<Xex2ImportRecord> importRecords;
    Image image = Xex2LoadImageEx(fileData.data(), fileData.size(),
                                  &importRecords);
    if (image.data == nullptr || image.size == 0 ||
        image.data[0] != 'M' || image.data[1] != 'Z') {
        __android_log_print(ANDROID_LOG_WARN, "FH2/XEX",
                            "XEX secundário '%s': decodificação falhou",
                            displayName.c_str());
        return 0;
    }
    // 3) tamanho REAL do conteúdo decodificado: o Xex2LoadImage devolve
    //    image.size = security->imageSize, mas o buffer descomprimido tem o
    //    tamanho da SOMA dos blocos (XEX_COMPRESSION_BASIC) — copiar
    //    image.size leria além do buffer (SIGSEGV no host / lixo no device).
    //    O restante do span (até security->imageSize) é zero-fill como no
    //    console (páginas presentes além do conteúdo = zero).
    const uint32_t contentBytes =
        secondaryContentBytes(fileData.data(), fileData.size());
    const uint32_t spanBytes =
        contentBytes < image.size ? image.size : contentBytes;

    // 4) colocação: BASE PREFERIDA do módulo (XEX_HEADER_IMAGE_BASE_ADDRESS
    //    — o Xex2LoadImage já a devolve em image.base). O console carrega o
    //    módulo na base preferida; o .reloc do XMediaFacade/SpeechFacade nem
    //    existe no arquivo (loader-consumido), logo relocar é impossível.
    //    Se a faixa estiver ocupada (alocação do título), cai para a região
    //    de módulos 0x98000000+ — comportamento honesto e logado.
    uint8_t* guestMem = fh2::kern::guestMem();
    const uint64_t ramBytes = fh2::kern::guestMemBytes();
    if (!guestMem || ramBytes == 0) return 0;
    const uint64_t align = 0x10000ull;
    uint64_t placed = 0;
    const uint64_t preferred = ((uint64_t)image.base + align - 1) & ~(align - 1);
    if (preferred >= 0x82000000ull &&
        preferred + spanBytes <= ramBytes &&
        fh2::kern::moduleRangeFree(preferred, spanBytes) &&
        fh2::kern::heap().reserveAt(preferred, spanBytes) != 0) {
        placed = preferred;
    } else {
        uint64_t candidate = (g_moduleRegionNext + align - 1) & ~(align - 1);
        while (candidate + spanBytes <= kModuleRegionBase + kModuleRegionSize &&
               !fh2::kern::moduleRangeFree(candidate, spanBytes)) {
            candidate += align;
        }
        if (candidate + spanBytes > kModuleRegionBase + kModuleRegionSize) {
            __android_log_print(ANDROID_LOG_ERROR, "FH2/XEX",
                                "região de módulos esgotada p/ '%s'",
                                displayName.c_str());
            return 0;
        }
        placed = candidate;
        __android_log_print(ANDROID_LOG_WARN, "FH2/XEX",
                            "'%s': base preferida 0x%08X ocupada — módulo em "
                            "0x%08llX (reloc sem .reloc: risco de ponteiros "
                            "absolutos)",
                            displayName.c_str(), image.base,
                            (unsigned long long)placed);
    }
    if (!fh2::kern::reserveModuleRange(placed, spanBytes)) {
        __android_log_print(ANDROID_LOG_ERROR, "FH2/XEX",
                            "faixa do módulo '%s' colidiu", displayName.c_str());
        return 0;
    }

    // 5) cópia do CONTEÚDO (o span restante já é zero na RAM guest — memfd
    //    zero-init + releaseSecondaryModules zera no relaunch)
    memcpy(guestMem + placed, image.data.get(), contentBytes);
    g_moduleRegionNext = placed + spanBytes;
    const uint32_t moduleVa = (uint32_t)placed;

    // 6) exports REAIS: (a) TABELA XEX2 NATIVA do security header — a
    //    estrutura que o console caminha em XexGetProcedureAddress (o
    //    XMediaFacade/SpeechFacade exportam 14/N ordinais por aqui); (b) PE
    //    EXPORT DIRECTORY — complemento para módulos que exportam só por PE.
    std::map<uint32_t, uint32_t> expMap = parseXex2ExportTable(
        fileData.data(), fileData.size(), image.data.get(), contentBytes);
    for (auto& [ord, va] : parseExportTable(image.data.get(),
                                            (uint32_t)image.size, moduleVa)) {
        expMap[ord] = va;
    }
    const uint32_t handle =
        fh2::kern::registerModule(displayName, 0, expMap);
    fh2::kern::markModuleSecondary(handle, moduleVa, (uint32_t)spanBytes);
    __android_log_print(ANDROID_LOG_INFO, "FH2/XEX",
                        "XEX secundário carregado: %s base=0x%08X (preferida "
                        "0x%08X) span=%u conteúdo=%u entry=0x%08X exports=%zu "
                        "(handle 0x%08X)",
                        displayName.c_str(), moduleVa, image.base,
                        spanBytes, contentBytes, image.entry_point,
                        expMap.size(), handle);
    return handle;
}

} // namespace fh2::ppc
