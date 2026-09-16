// fs_provider.cpp — leitura DIRETA da árvore SAF (sem cópia) + I/O atômico
#include "fs_provider.h"

#include <android/log.h>
#include <cstdio>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define FLOG(...) __android_log_print(ANDROID_LOG_INFO, "FH2/FS", __VA_ARGS__)
#define FLOGE(...) __android_log_print(ANDROID_LOG_ERROR, "FH2/FS", __VA_ARGS__)

namespace fh2::fs {

void FsProvider::initialize(JNIEnv* env) {
    if (!env) return;
    if (env->GetJavaVM(&jvm_) != JNI_OK) { jvm_ = nullptr; return; }

    // NativeBridge: fd direto (openSaf) + fallback de cópia (copyFromSaf)
    jclass cls = env->FindClass("com/fh2recomp/nativebridge/NativeBridge");
    if (!cls) { FLOGE("NativeBridge não encontrada p/ callbacks SAF"); env->ExceptionClear(); return; }
    bridgeClass_ = static_cast<jclass>(env->NewGlobalRef(cls));
    openSafMethod_ = env->GetStaticMethodID(cls, "openSaf", "(Ljava/lang/String;)I");
    if (!openSafMethod_) { FLOGE("openSaf não encontrado (leitura direta indisponível)"); env->ExceptionClear(); }
    copyMethod_ = env->GetStaticMethodID(cls, "copyFromSaf", "(Ljava/lang/String;)Z");
    if (!copyMethod_) { FLOGE("copyFromSaf não encontrado (fallback indisponível)"); env->ExceptionClear(); }
    bootMsgMethod_ = env->GetStaticMethodID(cls, "bootMessage", "(Ljava/lang/String;)V");
    if (!bootMsgMethod_) { FLOGE("bootMessage não encontrado (avisos de boot sem UI)"); env->ExceptionClear(); }
    env->DeleteLocalRef(cls);
}

// Anexa a thread atual à JVM se necessário (chamadas vêm da thread do guest).
JNIEnv* FsProvider::attachEnv(bool& attached) const {
    attached = false;
    if (!jvm_) return nullptr;
    JNIEnv* env = nullptr;
    if (jvm_->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6) != JNI_OK) {
        if (jvm_->AttachCurrentThread(&env, nullptr) != JNI_OK) return nullptr;
        attached = true;
    }
    return env;
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

void FsProvider::noteFailure(const std::string& p) const {
    std::lock_guard<std::mutex> lk(failM_);
    lastFailure_ = p;
}

std::string FsProvider::lastFailure() const {
    std::lock_guard<std::mutex> lk(failM_);
    return lastFailure_;
}

void FsProvider::showBootMessage(const std::string& msg) const {
    if (msg.empty() || !bridgeClass_ || !bootMsgMethod_) return;
    bool attached = false;
    JNIEnv* env = attachEnv(attached);
    if (!env) return;
    jstring jmsg = env->NewStringUTF(msg.c_str());
    if (jmsg) {
        env->CallStaticVoidMethod(bridgeClass_, bootMsgMethod_, jmsg);
        if (env->ExceptionCheck()) env->ExceptionClear();
        env->DeleteLocalRef(jmsg);
    }
    if (attached) jvm_->DetachCurrentThread();
}

bool FsProvider::resolvePath(const std::string& guestPath, std::string& out) const {
    if (!isSafeGuestPath(guestPath)) return false;
    if (filesDir_.empty()) return false;
    out = filesDir_ + "/" + guestPath;
    return true;
}

// fd DIRETO da árvore SAF: o Java resolve o caminho (cache de diretórios),
// abre via ContentResolver e devolve um fd detached. O native lê com pread e
// fecha — NENHUM byte é copiado para o storage do app.
int FsProvider::openSafDirect(const std::string& guestPath) const {
    bool attached = false;
    JNIEnv* env = attachEnv(attached);
    if (!env || !bridgeClass_ || !openSafMethod_) return -1;
    int fd = -1;
    jstring jpath = env->NewStringUTF(guestPath.c_str());
    if (jpath) {
        fd = env->CallStaticIntMethod(bridgeClass_, openSafMethod_, jpath);
        if (env->ExceptionCheck()) { env->ExceptionClear(); fd = -1; }
        env->DeleteLocalRef(jpath);
    }
    if (attached) jvm_->DetachCurrentThread();
    return fd;
}

// Último recurso (providers sem openFileDescriptor): copia via Java p/ filesDir.
bool FsProvider::copyFromSaf(const std::string& guestPath) const {
    bool attached = false;
    JNIEnv* env = attachEnv(attached);
    bool ok = false;
    if (env && bridgeClass_ && copyMethod_) {
        jstring jpath = env->NewStringUTF(guestPath.c_str());
        if (jpath) {
            ok = env->CallStaticBooleanMethod(bridgeClass_, copyMethod_, jpath) == JNI_TRUE;
            if (env->ExceptionCheck()) { env->ExceptionClear(); ok = false; }
            env->DeleteLocalRef(jpath);
        }
    }
    if (attached) jvm_->DetachCurrentThread();
    if (ok) FLOG("fallback cópia lazy: %s", guestPath.c_str());
    else FLOGE("leitura indisponível: %s", guestPath.c_str());
    return ok;
}

// Consome um fd inteiro (ou apenas sonda presença se maxBytes>0).
// Tolerante a fds não-seekable (providers virtuais): leitura incremental.
static bool readFdFully(int fd, std::vector<uint8_t>& out, size_t maxBytes) {
    if (maxBytes > 0) { out.clear(); return true; } // sonda: abriu = existe
    off_t end = lseek(fd, 0, SEEK_END);
    if (end >= 0) {
        size_t size = size_t(end);
        if (lseek(fd, 0, SEEK_SET) < 0) return false;
        out.resize(size);
        size_t total = 0;
        while (total < size) {
            ssize_t n = pread(fd, out.data() + total, size - total, off_t(total));
            if (n <= 0) return false;
            total += size_t(n);
        }
        return true;
    }
    out.clear();
    uint8_t buf[256 * 1024];
    ssize_t n;
    while ((n = read(fd, buf, sizeof(buf))) > 0)
        out.insert(out.end(), buf, buf + size_t(n));
    return !out.empty();
}

bool FsProvider::readLocal(const std::string& path, std::vector<uint8_t>& out,
                           size_t maxBytes) const {
    FILE* f = fopen(path.c_str(), "rb");
    if (!f) return false;
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
    return rd == size_t(size);
}

bool FsProvider::readFile(const std::string& guestPath, std::vector<uint8_t>& out,
                          size_t maxBytes) const {
    if (!isSafeGuestPath(guestPath)) return false;

    // (1) DIRETO da árvore SAF — caminho padrão, sem cópia para o app
    if (!assetsUri_.empty() && openSafMethod_) {
        int fd = openSafDirect(guestPath);
        if (fd >= 0) {
            bool ok = readFdFully(fd, out, maxBytes);
            close(fd);
            if (ok) return true;
            FLOGE("fd SAF devolveu dados incompletos: %s", guestPath.c_str());
        }
    }
    // (2) Local em filesDir — arquivos colocados manualmente ou cópias antigas
    std::string path;
    if (resolvePath(guestPath, path) && readLocal(path, out, maxBytes)) return true;

    // (3) Último recurso: cópia lazy via Java (provider sem openFileDescriptor)
    if (!assetsUri_.empty() && copyFromSaf(guestPath) &&
        resolvePath(guestPath, path) && readLocal(path, out, maxBytes)) return true;

    noteFailure(guestPath);
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

int FsProvider::openFileFd(const std::string& guestPath, uint64_t& sizeOut) const {
    if (!isSafeGuestPath(guestPath)) return -1;
    // (1) fd DIRETO da árvore SAF — seekable, pread funciona sem cópia
    if (!assetsUri_.empty() && openSafMethod_) {
        int fd = openSafDirect(guestPath);
        if (fd >= 0) {
            struct stat st;
            if (fstat(fd, &st) == 0 && S_ISREG(st.st_mode)) {
                sizeOut = (uint64_t)st.st_size;
                return fd;
            }
            close(fd);
        }
    }
    // (2) arquivo local em filesDir (ou cópia lazy anterior)
    std::string path;
    if (resolvePath(guestPath, path)) {
        int fd = open(path.c_str(), O_RDONLY);
        if (fd >= 0) {
            struct stat st;
            if (fstat(fd, &st) == 0 && S_ISREG(st.st_mode)) {
                sizeOut = (uint64_t)st.st_size;
                return fd;
            }
            close(fd);
        }
        // (3) provider sem openFileDescriptor: cópia lazy e reabre
        if (!assetsUri_.empty() && copyFromSaf(guestPath)) {
            fd = open(path.c_str(), O_RDONLY);
            if (fd >= 0) {
                struct stat st;
                if (fstat(fd, &st) == 0 && S_ISREG(st.st_mode)) {
                    sizeOut = (uint64_t)st.st_size;
                    return fd;
                }
                close(fd);
            }
        }
    }
    noteFailure(guestPath);
    return -1;
}

int FsProvider::openLocalWriteFd(const std::string& guestPath) const {
    if (!isSafeGuestPath(guestPath)) return -1;
    std::string path;
    if (!resolvePath(guestPath, path)) return -1;
    // garante diretórios intermediários
    for (size_t pos = filesDir_.size() + 1; pos < path.size(); ++pos) {
        if (path[pos] == '/') mkdir(path.substr(0, pos).c_str(), 0755);
    }
    return open(path.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
}

} // namespace fh2::fs
