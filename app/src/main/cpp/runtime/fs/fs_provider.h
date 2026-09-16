// fs_provider.h — abstração de I/O para storage Android
//
// O usuário seleciona a pasta de assets via SAF (ACTION_OPEN_DOCUMENT_TREE).
// LEITURA: DIRETA da árvore SAF — o Java (NativeBridge.openSaf) resolve o
// caminho, abre via ContentResolver e devolve um fd cru (detachFd); o native
// lê com pread e fecha. Nenhum byte é copiado para o storage do app.
// Fallbacks, em ordem: arquivo local em filesDir (colocado manualmente) e
// cópia lazy via Java para providers sem openFileDescriptor.
// ESCRITA (saves, shader cache): fica no app-specific storage, atômica
// (tmp + rename) — a pasta escolhida não é modificada.
#pragma once

#include <jni.h>
#include <mutex>
#include <string>
#include <vector>

namespace fh2::fs {

class FsProvider {
public:
    /** Registra o URI SAF persistido (chamado do boot com o valor do intent). */
    void setAssetsTreeUri(const std::string& uri) { assetsUri_ = uri; }

    /** Configura o diretório de arquivos do app (getFilesDir via JNI). */
    void setFilesDir(const std::string& path) { filesDir_ = path; }

    /** Inicializa JavaVM refs para os callbacks SAF (fd direto + fallback). */
    void initialize(JNIEnv* env);

    /**
     * Lê um arquivo do storage do jogo (retorna false se ausente).
     * Ordem: (1) fd DIRETO da árvore SAF — zero cópia; (2) caminho local em
     * filesDir; (3) cópia lazy via Java (último recurso).
     * maxBytes>0 = sondagem de presença (não carrega o conteúdo).
     */
    bool readFile(const std::string& guestPath, std::vector<uint8_t>& out,
                  size_t maxBytes = 0) const;

    /** Escreve arquivo (saves, shader cache persistente) — atômico. */
    bool writeFile(const std::string& guestPath, const std::vector<uint8_t>& data) const;

    /**
     * Abre um arquivo para I/O aleatório REAL (pread): fd cru + tamanho.
     * Ordem: árvore SAF (fd do ContentResolver — seekable) → caminho local
     * em filesDir. Retorna -1 se o arquivo não existir. O fd é de propriedade
     * do chamador (close()).
     */
    int openFileFd(const std::string& guestPath, uint64_t& sizeOut) const;

    /** Abre arquivo LOCAL (filesDir) para escrita — cria diretórios. Usado
     *  pelo NtWriteFile (saves). -1 se o caminho não for seguro. */
    int openLocalWriteFd(const std::string& guestPath) const;

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
    JNIEnv* attachEnv(bool& attached) const;

    std::string assetsUri_;       // content:// URI SAF (leitura direta)
    std::string filesDir_;        // app-specific storage (escrita + fallback)
    JavaVM* jvm_ = nullptr;       // para anexar a thread do guest no callback
    jclass bridgeClass_ = nullptr;    // GlobalRef de NativeBridge
    jmethodID openSafMethod_ = nullptr; // openSaf(String)I — fd direto
    jmethodID copyMethod_ = nullptr;    // copyFromSaf(String)Z — fallback
    jmethodID bootMsgMethod_ = nullptr; // bootMessage(String)V — Toast UI
    mutable std::mutex failM_;          // protege lastFailure_ (guest threads)
    mutable std::string lastFailure_;   // último caminho não lido
};

} // namespace fh2::fs
