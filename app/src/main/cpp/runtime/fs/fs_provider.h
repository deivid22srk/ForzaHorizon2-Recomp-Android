// fs_provider.h — abstração de I/O para storage Android
//
// O usuário seleciona a pasta de assets via SAF (ACTION_OPEN_DOCUMENT_TREE).
// O URI persistido é resolvido sob demanda: readFile() tenta o caminho local
// (app-specific storage) e, se ausente, pede à camada Java (StaticBridge) que
// copie o arquivo da árvore SAF — sem copiar o jogo inteiro antecipadamente.
// Escrita (saves, shader cache) é atômica (tmp + rename).
#pragma once

#include <jni.h>
#include <string>
#include <vector>

namespace fh2::fs {

class FsProvider {
public:
    /** Registra o URI SAF persistido (chamado do boot com o valor do intent). */
    void setAssetsTreeUri(const std::string& uri) { assetsUri_ = uri; }

    /** Configura o diretório de arquivos do app (getFilesDir via JNI). */
    void setFilesDir(const std::string& path) { filesDir_ = path; }

    /** Inicializa JavaVM refs para o callback lazy de cópia. */
    void initialize(JNIEnv* env);

    /**
     * Lê um arquivo do storage do jogo (retorna false se ausente).
     * Resolução: filesDir primeiro; se ausente e há URI SAF, dispara cópia
     * lazy via Java e re-tenta. maxBytes>0 = sondagem (não lê o conteúdo).
     */
    bool readFile(const std::string& guestPath, std::vector<uint8_t>& out,
                  size_t maxBytes = 0) const;

    /** Escreve arquivo (saves, shader cache persistente) — atômico. */
    bool writeFile(const std::string& guestPath, const std::vector<uint8_t>& data) const;

    const std::string& filesDir() const { return filesDir_; }

private:
    bool resolvePath(const std::string& guestPath, std::string& out) const;
    bool copyFromSaf(const std::string& guestPath) const;

    std::string assetsUri_;   // content:// URI SAF (leitura sob demanda)
    std::string filesDir_;    // app-specific storage (leitura pós-cópia + escrita)
    JavaVM* jvm_ = nullptr;   // para anexar a thread do guest no callback
    jobject bridgeRef_ = nullptr; // GlobalRef da NativeBridge (callback)
    jmethodID copyMethod_ = nullptr;
};

} // namespace fh2::fs
