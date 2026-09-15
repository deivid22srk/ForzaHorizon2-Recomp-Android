// fs_provider.cpp — resolução SAF (cópia lazy) + I/O atômico
#include "fs_provider.h"

#include <android/log.h>
#include <cstdio>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define FLOG(...) __android_log_print(ANDROID_LOG_INFO, "FH2/FS", __VA_ARGS__)
#define FLOGE(...) __android_log_print(ANDROID_LOG_ERROR, "FH2/FS", __VA_ARGS__)

namespace fh2::fs {

void FsProvider::initialize(JNIEnv* env) {
    if (!env) return;
    if (env->GetJavaVM(&jvm_) != JNI_OK) { jvm_ = nullptr; return; }

    // NativeBridge (classe do callback de cópia lazy)
    jclass cls = env->FindClass("com/fh2recomp/nativebridge/NativeBridge");
    if (!cls) { FLOGE("NativeBridge não encontrada p/ callback SAF"); env->ExceptionClear(); return; }
    copyMethod_ = env->GetStaticMethodID(cls, "copyFromSaf", "(Ljava/lang/String;)Z");
    if (!copyMethod_) { FLOGE("método copyFromSaf não encontrado"); env->ExceptionClear(); }
    env->DeleteLocalRef(cls);
}

// Rejeita traversal: "..", caminhos absolutos e componentes vazios
static bool isSafeGuestPath(const std::string& p) {
    if (p.empty() || p[0] == '/') return false;
    size_t start = 0;
    while (start <= p.size()) {
        size_t end = p.find('/', start);
        std::string part = p.substr(start, end == std::string::npos ? std::string::npos : end - start);
        if (part.empty() || part == "." || part == "..") return false;
        if (end == std::string::npos) break;
        start = end + 1;
    }
    return true;
}

bool FsProvider::resolvePath(const std::string& guestPath, std::string& out) const {
    if (!isSafeGuestPath(guestPath)) return false;
    if (filesDir_.empty()) return false;
    out = filesDir_ + "/" + guestPath;
    return true;
}

bool FsProvider::copyFromSaf(const std::string& guestPath) const {
    if (!jvm_ || !copyMethod_) return false;
    JNIEnv* env = nullptr;
    bool attached = false;
    if (jvm_->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6) != JNI_OK) {
        if (jvm_->AttachCurrentThread(&env, nullptr) != JNI_OK) return false;
        attached = true;
    }
    bool ok = false;
    if (env) {
        jstring jpath = env->NewStringUTF(guestPath.c_str());
        if (jpath) {
            ok = env->CallStaticBooleanMethod(env->FindClass("com/fh2recomp/nativebridge/NativeBridge"),
                                              copyMethod_, jpath) == JNI_TRUE;
            if (env->ExceptionCheck()) { env->ExceptionClear(); ok = false; }
            env->DeleteLocalRef(jpath);
        }
        jclass cls = env->FindClass("com/fh2recomp/nativebridge/NativeBridge");
        if (cls) env->DeleteLocalRef(cls);
    }
    if (attached) jvm_->DetachCurrentThread();
    if (ok) FLOG("cópia lazy SAF ok: %s", guestPath.c_str());
    else FLOGE("cópia lazy SAF falhou: %s", guestPath.c_str());
    return ok;
}

bool FsProvider::readFile(const std::string& guestPath, std::vector<uint8_t>& out,
                          size_t maxBytes) const {
    for (int attempt = 0; attempt < 2; ++attempt) {
        std::string path;
        if (!resolvePath(guestPath, path)) return false;
        FILE* f = fopen(path.c_str(), "rb");
        if (!f) {
            if (attempt == 0 && !assetsUri_.empty() && copyFromSaf(guestPath)) continue;
            return false;
        }
        fseek(f, 0, SEEK_END);
        long size = ftell(f);
        fseek(f, 0, SEEK_SET);
        if (size < 0) { fclose(f); return false; }
        if (maxBytes > 0) {
            // sondagem de presença: não carrega conteúdo
            fclose(f);
            out.resize(0);
            return true;
        }
        out.resize(size_t(size));
        size_t rd = fread(out.data(), 1, size_t(size), f);
        fclose(f);
        if (rd == size_t(size)) return true;
    }
    return false;
}

bool FsProvider::writeFile(const std::string& guestPath, const std::vector<uint8_t>& data) const {
    std::string path;
    if (!resolvePath(guestPath, path)) return false;
    // garante diretórios intermediários
    for (size_t pos = filesDir_.size() + 1; pos < path.size(); ++pos) {
        if (path[pos] == '/') mkdir(path.substr(0, pos).c_str(), 0755);
    }
    // escrita atômica: tmp + rename
    std::string tmp = path + ".tmp";
    FILE* f = fopen(tmp.c_str(), "wb");
    if (!f) return false;
    size_t wr = data.empty() ? 0 : fwrite(data.data(), 1, data.size(), f);
    if (fflush(f) != 0 || fclose(f) != 0 || wr != data.size()) {
        unlink(tmp.c_str());
        return false;
    }
    if (rename(tmp.c_str(), path.c_str()) != 0) {
        unlink(tmp.c_str());
        return false;
    }
    return true;
}

} // namespace fh2::fs
