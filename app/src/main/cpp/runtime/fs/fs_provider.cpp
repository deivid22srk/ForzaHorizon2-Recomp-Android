// fs_provider.cpp — resolução SAF + I/O de arquivos
#include "fs_provider.h"

#include <android/log.h>
#include <cstdio>
#include <sys/stat.h>
#include <unistd.h>

#define FLOG(...) __android_log_print(ANDROID_LOG_INFO, "FH2/FS", __VA_ARGS__)

namespace fh2::fs {

void FsProvider::initialize(JNIEnv* env, const char* assetsTreeUri) {
    if (assetsTreeUri && *assetsTreeUri) {
        // Nota: SAF não expõe paths diretos; a camada Java sincroniza a árvore
        // selecionada para o app-specific storage (cópia sob demanda por chunk).
        // Aqui registramos o URI para a resolução guest→host.
        assetsBase_ = assetsTreeUri;
        FLOG("assets URI registrado: %s", assetsBase_.c_str());
    }
    const char* appFiles = "/data/data/com.fh2recomp/files";
    mkdir(appFiles, 0755);
    writableBase_ = appFiles;
    FLOG("writable base: %s", writableBase_.c_str());
}

bool FsProvider::readFile(const std::string& guestPath, std::vector<uint8_t>& out) const {
    // Resolve: writable primeiro (saves/cache), depois assets
    for (const std::string* base : {&writableBase_, &assetsBase_}) {
        std::string path = *base + "/" + guestPath;
        FILE* f = fopen(path.c_str(), "rb");
        if (!f) continue;
        fseek(f, 0, SEEK_END);
        long size = ftell(f);
        fseek(f, 0, SEEK_SET);
        if (size < 0) { fclose(f); continue; }
        out.resize(size_t(size));
        size_t rd = fread(out.data(), 1, size_t(size), f);
        fclose(f);
        if (rd == size_t(size)) return true;
    }
    return false;
}

bool FsProvider::writeFile(const std::string& guestPath, const std::vector<uint8_t>& data) const {
    std::string path = writableBase_ + "/" + guestPath;
    FILE* f = fopen(path.c_str(), "wb");
    if (!f) return false;
    size_t wr = fwrite(data.data(), 1, data.size(), f);
    fclose(f);
    return wr == data.size();
}

} // namespace fh2::fs
