// xiso.h — leitura DIRETA de imagem de disco Xbox 360 (GDFX/XDVDFS)
//
// Formato REAL do disco do 360 (mesmo layout usado pelo Xenia —
// vfs/devices/disc_image_device.cc + gdfx_util.h, BSD, apenas como
// documentação do formato):
//   • setor de 2048 bytes;
//   • a partição do jogo começa em um dos offsets conhecidos
//     (0x0, 0xFB20, 0x20600, 0x2080000, 0xFD90000) e o magic
//     "MICROSOFT*XBOX*MEDIA" fica no setor 32 da partição;
//   • no setor 32: root_dir_sector (u32 LE @+20), root_dir_size (u32 LE @+24);
//   • diretório = árvore binária de entradas com stride de 4 bytes:
//       +0 u16 node_l   +2 u16 node_r   +4 u32 sector   +8 u32 length
//       +12 u8 attributes (0x10 = diretório)   +13 u8 name_len   +14 name
//     campos LITTLE-endian no disco (o guest acessa via kernel, não via RAM);
//   • arquivo: dados em gameOffset + sector*2048, `length` bytes.
//
// A imagem é lida por pread no fd SAF (zero cópia) e o fd permanece aberto
// para I/O aleatório dos arquivos do jogo (default.xex, media.zip, ...).
#pragma once

#include <cstdint>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include <unistd.h> // ::dup

namespace fh2::fs {

class XisoImage {
public:
    XisoImage() = default;
    ~XisoImage();

    XisoImage(const XisoImage&) = delete;
    XisoImage& operator=(const XisoImage&) = delete;

    /** Assume a posse do fd (pread aleatório) e monta a árvore GDFX. */
    bool open(int fd);

    bool valid() const { return fd_ >= 0 && gameOffset_ != kInvalidOff; }
    uint64_t totalSize() const { return total_; }
    uint64_t gameOffset() const { return gameOffset_; }

    /** Novo fd independente para a mesma imagem (pread aleatório). */
    int dupFd() const { return fd_ >= 0 ? ::dup(fd_) : -1; }

    struct Node {
        uint64_t offset = 0; // absoluto no arquivo ISO
        uint64_t size = 0;
        bool dir = false;
    };

    /** Busca por caminho relativo ("media.zip", "xmedia/a.xex"). */
    bool lookup(const std::string& relPath, Node& out) const;

    /** Lê o arquivo inteiro (ou maxBytes iniciais quando maxBytes>0). */
    bool readFile(const std::string& relPath, std::vector<uint8_t>& out,
                  size_t maxBytes = 0) const;

    /** Leitura bruta (disco \Device\Harddisk0\Partition0). */
    size_t readRaw(uint64_t byteOffset, uint8_t* dst, size_t len) const;

private:
    static constexpr uint64_t kInvalidOff = ~0ull;
    static constexpr uint32_t kSector = 2048;

    void parseTree(uint64_t dirOffsetAbs, uint32_t dirSize,
                   const std::string& prefix, int depth);
    bool readAt(uint64_t off, void* dst, size_t len) const;

    int fd_ = -1;
    uint64_t total_ = 0;
    uint64_t gameOffset_ = kInvalidOff;
    std::unordered_map<std::string, Node> files_; // chave em minúsculas
    mutable std::mutex ioM_; // pread é atômico, mas serialize p/ clareza
};

} // namespace fh2::fs
