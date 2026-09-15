// fs_provider.h — abstração de I/O para storage Android
//
// O usuário seleciona a pasta de assets via SAF (ACTION_OPEN_DOCUMENT_TREE).
// O URI persistido é resolvido para paths legíveis pelo guest; arquivos de
// sistema (app-specific storage) ficam em /data/data/com.fh2recomp/files.
#pragma once

#include <jni.h>
#include <string>
#include <vector>

namespace fh2::fs {

class FsProvider {
public:
    /** Resolve o URI SAF persistido para um path base utilizável. */
    void initialize(JNIEnv* env, const char* assetsTreeUri);

    /** Lê um arquivo do storage do jogo (retorna false se ausente). */
    bool readFile(const std::string& guestPath, std::vector<uint8_t>& out) const;

    /** Escreve arquivo (saves, shader cache persistente). */
    bool writeFile(const std::string& guestPath, const std::vector<uint8_t>& data) const;

    const std::string& assetsBasePath() const { return assetsBase_; }
    const std::string& writableBasePath() const { return writableBase_; }

private:
    std::string assetsBase_;    // storage do usuário (leitura)
    std::string writableBase_;  // app-specific storage (escrita: saves, shader cache)
};

} // namespace fh2::fs
