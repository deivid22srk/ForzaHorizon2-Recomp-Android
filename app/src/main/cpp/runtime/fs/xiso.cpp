// xiso.cpp — implementação do parser GDFX/XDVDFS (formato real do disco 360).
#include "xiso.h"

#include <android/log.h>

#include <algorithm>
#include <cstring>

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define XLOG(...) __android_log_print(ANDROID_LOG_INFO, "FH2/XISO", __VA_ARGS__)
#define XERR(...) __android_log_print(ANDROID_LOG_ERROR, "FH2/XISO", __VA_ARGS__)

namespace fh2::fs {

namespace {

constexpr char kMagic[20] = {'M', 'I', 'C', 'R', 'O', 'S', 'O', 'F', 'T', '*',
                             'X', 'B', 'O', 'X', '*', 'M', 'E', 'D', 'I', 'A'};

// Offsets onde a partição do jogo pode começar (discos com security sector
// deslocado / imagens de recarga — mesmos offsets conhecidos do Xenia).
constexpr uint64_t kLikelyOffsets[] = {
    0x00000000ull, 0x0000FB20ull, 0x00020600ull, 0x02080000ull, 0x0FD90000ull,
};

inline uint16_t rdU16(const uint8_t* p) {
    return uint16_t(p[0]) | (uint16_t(p[1]) << 8);
}
inline uint32_t rdU32(const uint8_t* p) {
    return uint32_t(p[0]) | (uint32_t(p[1]) << 8) | (uint32_t(p[2]) << 16) |
           (uint32_t(p[3]) << 24);
}

std::string lowerAscii(const std::string& s) {
    std::string out = s;
    for (char& c : out) {
        if (c >= 'A' && c <= 'Z') c = char(c - 'A' + 'a');
    }
    return out;
}

std::string joinKey(const std::string& prefix, const char* name, size_t n) {
    std::string out;
    out.reserve(prefix.size() + n + 1);
    if (!prefix.empty()) {
        out += prefix;
        out += '/';
    }
    out.append(name, n);
    return lowerAscii(out);
}

} // namespace

XisoImage::~XisoImage() {
    if (fd_ >= 0) close(fd_);
}

bool XisoImage::readAt(uint64_t off, void* dst, size_t len) const {
    if (fd_ < 0 || off + len > total_) return false;
    uint8_t* d = (uint8_t*)dst;
    size_t done = 0;
    while (done < len) {
        const ssize_t n = pread(fd_, d + done, len - done,
                                (off_t)(off + done));
        if (n <= 0) return false;
        done += (size_t)n;
    }
    return true;
}

bool XisoImage::open(int fd) {
    if (fd_ >= 0) close(fd_);
    fd_ = fd;
    files_.clear();
    gameOffset_ = kInvalidOff;
    if (fd_ < 0) return false;

    struct stat st;
    if (fstat(fd_, &st) != 0 || !S_ISREG(st.st_mode)) {
        XERR("ISO: fd não é arquivo regular");
        return false;
    }
    total_ = (uint64_t)st.st_size;
    XLOG("ISO: %llu bytes abertos via fd direto (zero cópia)",
         (unsigned long long)total_);

    // Localiza a partição do jogo: magic no setor 32 de cada offset provável.
    for (uint64_t off : kLikelyOffsets) {
        const uint64_t sector32 = off + 32ull * kSector;
        if (sector32 + 28 > total_) continue;
        uint8_t sec[2048];
        if (!readAt(sector32, sec, sizeof(sec))) continue;
        if (memcmp(sec, kMagic, sizeof(kMagic)) != 0) continue;
        const uint32_t rootSector = rdU32(sec + 20);
        const uint32_t rootSize = rdU32(sec + 24);
        if (rootSize < 13 || rootSize > 32u * 1024 * 1024) continue;
        const uint64_t rootAbs = off + (uint64_t)rootSector * kSector;
        if (rootAbs + rootSize > total_) continue;
        gameOffset_ = off;
        XLOG("ISO: partição do jogo em 0x%08llX (raiz setor %u, %u bytes)",
             (unsigned long long)off, rootSector, rootSize);
        parseTree(rootAbs, rootSize, "", 0);
        break;
    }

    if (gameOffset_ == kInvalidOff) {
        XERR("ISO: nenhuma partição GDFX/XDVDFS encontrada — não é uma "
             "imagem de jogo Xbox 360");
        return false;
    }
    XLOG("ISO: árvore montada — %zu arquivos/diretórios indexados",
         files_.size());
    return true;
}

void XisoImage::parseTree(uint64_t dirOffsetAbs, uint32_t dirSize,
                          const std::string& prefix, int depth) {
    if (depth > 32) return; // defesa contra árvore corrompida
    if (dirSize == 0 || dirOffsetAbs + dirSize > total_) return;

    // Buffer do diretório (leitura única; árvores do FH2 cabem na RAM).
    std::vector<uint8_t> buf(dirSize);
    if (!readAt(dirOffsetAbs, buf.data(), dirSize)) return;

    // Árvore binária XDVDFS — convenção REAL do console (idêntica ao parser
    // do Xenia, disc_image_device.cc): ordinais ABSOLUTOS em words (×4) do
    // buffer do diretório, raiz na word 0, visita em-ordem (esq, nó, dir).
    // Validação por entrada: nome ASCII imprimível 1..255 e arquivo dentro
    // da imagem — entradas corrompidas são descartadas, não indexadas.
    const uint32_t dirWords = dirSize / 4;

    auto validName = [](const uint8_t* p, uint8_t len) {
        if (len == 0 || len > 255) return false;
        for (uint8_t i = 0; i < len; ++i) {
            const uint8_t c = p[i];
            if (c < 0x20 || c > 0x7E) return false;
        }
        return true;
    };

    struct Frame {
        uint16_t ordinal;
        const uint8_t* base;
        std::string prefix;
    };
    std::vector<Frame> stack;
    std::vector<bool> visited(dirWords, false);
    stack.push_back({0, buf.data(), prefix});

    while (!stack.empty()) {
        const Frame fr = stack.back();
        stack.pop_back();
        if (fr.ordinal >= dirWords) continue;
        if (visited[fr.ordinal]) continue;
        visited[fr.ordinal] = true;

        const uint64_t entryOff = (uint64_t)fr.ordinal * 4;
        const uint8_t* p = fr.base + entryOff;
        if (entryOff + 14 > dirSize) continue;

        const uint16_t nodeL = rdU16(p + 0);
        const uint16_t nodeR = rdU16(p + 2);
        const uint32_t sector = rdU32(p + 4);
        const uint32_t length = rdU32(p + 8);
        const uint8_t attrs = p[12];
        const uint8_t nameLen = p[13];
        const bool isDir = (attrs & 0x10) != 0;
        if (!validName(p + 14, nameLen)) continue;
        if (entryOff + 14 + nameLen > dirSize) continue;

        const std::string name((const char*)p + 14, nameLen);
        const std::string key = joinKey(fr.prefix, name.data(), name.size());

        Node node;
        node.dir = isDir;
        node.size = length;
        node.offset = isDir ? 0
                            : gameOffset_ + (uint64_t)sector * kSector;
        // arquivos de tamanho 0 são REAIS (marcadores do disco) e ficam
        // indexados; só descarta arquivo cujos dados saem da imagem
        if (!isDir && node.offset + node.size > total_) {
            continue; // entrada fora da imagem — corrompida, ignora
        }
        if (files_.find(key) == files_.end()) {
            files_.emplace(std::move(key), node);
        }

        // subdiretório: a árvore filha vive em gameOffset+sector*2048
        if (isDir && length > 0) {
            const uint64_t childAbs = gameOffset_ + (uint64_t)sector * kSector;
            parseTree(childAbs, length, key, depth + 1);
        }

        // esquerda primeiro na pilha = direita processada antes — a indexação
        // é por chave (map), a ordem não altera o resultado
        if (nodeL != 0xFFFF && nodeL != 0) stack.push_back({nodeL, fr.base, fr.prefix});
        if (nodeR != 0xFFFF && nodeR != 0) stack.push_back({nodeR, fr.base, fr.prefix});
    }
}

bool XisoImage::lookup(const std::string& relPath, Node& out) const {
    const auto it = files_.find(lowerAscii(relPath));
    if (it == files_.end() || it->second.dir) return false;
    out = it->second;
    return true;
}

bool XisoImage::readFile(const std::string& relPath, std::vector<uint8_t>& out,
                         size_t maxBytes) const {
    Node n;
    if (!lookup(relPath, n)) return false;
    const size_t want = maxBytes && maxBytes < n.size ? maxBytes : (size_t)n.size;
    out.resize(want);
    if (want == 0) return true;
    return readAt(n.offset, out.data(), want);
}

size_t XisoImage::readRaw(uint64_t byteOffset, uint8_t* dst, size_t len) const {
    if (byteOffset >= total_) return 0;
    const size_t want = (size_t)std::min<uint64_t>(len, total_ - byteOffset);
    return readAt(byteOffset, dst, want) ? want : 0;
}

} // namespace fh2::fs
