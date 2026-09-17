// fs_provider.h — abstração de I/O para storage Android
//
// O usuário seleciona os assets do jogo de DUAS formas (ambas SEM cópia):
//   • PASTA via SAF (ACTION_OPEN_DOCUMENT_TREE): leitura direta — o Java
//     (NativeBridge.openSaf) resolve o caminho, abre via ContentResolver e
//     devolve um fd cru (detachFd); o native lê com pread e fecha.
//   • ISO do jogo (ACTION_OPEN_DOCUMENT): fd direto do arquivo .iso, com a
//     árvore GDFX/XDVDFS (xiso.h) montada no native — os arquivos do disco
//     (default.xex, media.zip, ...) são servidos por pread no offset do
//     arquivo dentro da imagem. Nenhum byte é copiado para o app.
// Fallbacks, em ordem: arquivo local em filesDir (colocado manualmente) e
// cópia lazy via Java para providers sem openFileDescriptor.
// ESCRITA (saves, shader cache): fica no app-specific storage, atômica
// (tmp + rename) — a pasta/ISO escolhida não é modificada.
#pragma once

#include <jni.h>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "xiso.h"

namespace fh2::fs {

class FsProvider {
public:
    /** Registra o URI SAF persistido (chamado do boot com o valor do intent). */
    void setAssetsTreeUri(const std::string& uri) { assetsUri_ = uri; }

    /** Modo da origem: false = pasta (árvore SAF), true = arquivo ISO. */
    void setAssetsIsIso(bool iso) { assetsIsIso_ = iso; }
    bool isIsoMode() const { return assetsIsIso_; }

    /** Configura o diretório de arquivos do app (getFilesDir via JNI). */
    void setFilesDir(const std::string& path) { filesDir_ = path; }

    /** Modo host (harness de desktop): abre a ISO por caminho de arquivo
     *  local em vez do URI SAF. Mantém a MESMA árvore GDFX e o MESMO
     *  pipeline de leitura (pread no fd) — nenhuma diferença semântica. */
    void setLocalIsoPath(const std::string& path) { localIsoPath_ = path; }

    /** Inicializa JavaVM refs para os callbacks SAF (fd direto + fallback). */
    void initialize(JNIEnv* env);

    /**
     * Lê um arquivo do storage do jogo (retorna false se ausente).
     * Modo ISO: leitura DIRETA da imagem (pread no offset do arquivo).
     * Modo pasta: (1) fd DIRETO da árvore SAF — zero cópia; (2) caminho
     * local em filesDir; (3) cópia lazy via Java (último recurso).
     * maxBytes>0 = sondagem de presença (não carrega o conteúdo).
     */
    bool readFile(const std::string& guestPath, std::vector<uint8_t>& out,
                  size_t maxBytes = 0) const;

    /** Escreve arquivo (saves, shader cache persistente) — atômico. */
    bool writeFile(const std::string& guestPath, const std::vector<uint8_t>& data) const;

    /**
     * Abre um arquivo para I/O aleatório REAL (pread): fd cru + tamanho +
     * offset-base dentro do fd (ISO: offset do arquivo na imagem; pasta: 0).
     * O fd é de propriedade do chamador (close()). -1 se não existir.
     */
    int openFileFdEx(const std::string& guestPath, uint64_t& sizeOut,
                     uint64_t& baseOffsetOut) const;

    /** Compatibilidade: openFileFdEx com base 0. */
    int openFileFd(const std::string& guestPath, uint64_t& sizeOut) const {
        uint64_t base = 0;
        return openFileFdEx(guestPath, sizeOut, base);
    }

    /** fd do DISCO BRUTO (\Device\Harddisk0\Partition0): modo ISO devolve
     *  um dup do fd da imagem com o tamanho real do arquivo; -1 fora dele. */
    int openRawDiscFd(uint64_t& sizeOut) const;

    /** Abre arquivo LOCAL (filesDir) para escrita — cria diretórios. Usado
     *  pelo NtWriteFile (saves). -1 se o caminho não for seguro. */
    int openLocalWriteFd(const std::string& guestPath) const;

    /** Abre arquivo LOCAL (filesDir) para LEITURA+ESCRITA sem truncar —
     *  usado pelos dispositivos de bloco persistentes (partições de cache
     *  / HD do console mapeados em arquivos reais no storage do app). */
    int openLocalReadWriteFd(const std::string& guestPath) const;

    const std::string& filesDir() const { return filesDir_; }

    /** Último caminho cuja leitura falhou (todas as origens) — diagnóstico
     *  do loop de relaunch (XamLoaderLaunchTitle por arquivo ausente). */
    std::string lastFailure() const;

    /** Mensagem visível ao usuário (Toast via NativeBridge.bootMessage) —
     *  usada quando o boot falha por arquivo ausente, para que a falha NÃO
     *  seja uma tela preta silenciosa. */
    void showBootMessage(const std::string& msg) const;

private:
    void noteFailure(const std::string& p) const;
    bool resolvePath(const std::string& guestPath, std::string& out) const;
    bool readLocal(const std::string& path, std::vector<uint8_t>& out,
                   size_t maxBytes) const;
    int openSafDirect(const std::string& guestPath) const; // fd via Java (-1 = falhou)
    bool copyFromSaf(const std::string& guestPath) const;  // fallback legado
    XisoImage* ensureIso() const; // abre a ISO 1× (thread-safe; nullptr = erro)
    JNIEnv* attachEnv(bool& attached) const;

    std::string assetsUri_;       // content:// URI SAF (árvore OU arquivo ISO)
    bool assetsIsIso_ = false;    // true = URI é um arquivo .iso selecionado
    std::string filesDir_;        // app-specific storage (escrita + fallback)
    std::string localIsoPath_;    // modo host: caminho da ISO no filesystem
    JavaVM* jvm_ = nullptr;       // para anexar a thread do guest no callback
    jclass bridgeClass_ = nullptr;    // GlobalRef de NativeBridge
    jmethodID openSafMethod_ = nullptr; // openSaf(String)I — fd direto
    jmethodID openSafUriMethod_ = nullptr; // openSafUri(String)I — fd do URI
    jmethodID copyMethod_ = nullptr;    // copyFromSaf(String)Z — fallback
    jmethodID bootMsgMethod_ = nullptr; // bootMessage(String)V — Toast UI
    mutable std::mutex failM_;          // protege lastFailure_ (guest threads)
    mutable std::string lastFailure_;   // último caminho não lido

    // ISO aberta (lazy, thread-safe) — fd permanece aberto p/ pread aleatório
    mutable std::mutex isoM_;
    mutable std::unique_ptr<XisoImage> iso_;
    mutable bool isoTried_ = false;
};

} // namespace fh2::fs
