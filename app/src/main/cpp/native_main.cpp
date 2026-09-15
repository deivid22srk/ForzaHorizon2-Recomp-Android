// native_main.cpp — ponte JNI do runtime FH2-Recomp (Android, mobile-first)
//
// Responsabilidades:
//  - Ciclo de vida (boot/pause/resume/stop/superfície) vindo da GameActivity
//  - Fila de input (HUD touch + gamepad físico) consumida pelo guest
//  - Bootstrap do PPC runtime e da thread principal do jogo recompilado
//
// Concorrência: boot roda na thread "fh2-boot" (GameSurfaceView) enquanto
// stop/pause/resume/surface chegam da UI thread. Todo acesso ao estado global
// é serializado por lifecycleMutex_ para eliminar as corridas boot×stop e
// surface×stop. A thread do jogo só lê estado após o publish booted=true.
//
// O jogo recompilado só existe se recomp/generated estiver presente
// (FH2_HAS_RECOMP=1). Sem ele, o app opera em modo shell (setup do usuário).

#include <android/log.h>
#include <android/native_window.h>
#include <android/native_window_jni.h>
#include <jni.h>
#include <atomic>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

#include "runtime/gfx/graphics_backend.h"
#include "runtime/audio/audio_output.h"
#include "runtime/input/input_state.h"
#include "runtime/fs/fs_provider.h"
#include "runtime/ppc/ppc_runtime.h"
#include "runtime/ppc/kernel_state.h"

#define LOG_TAG "FH2Recomp"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

namespace {

struct RuntimeState {
    std::mutex lifecycleMutex;              // serializa boot/stop/surface
    std::atomic<bool> booted{false};
    std::atomic<bool> paused{false};
    std::atomic<bool> stopRequested{false};
    std::unique_ptr<fh2::gfx::GraphicsBackend> gfx;
    std::unique_ptr<fh2::audio::AudioOutput> audio;
    std::unique_ptr<fh2::ppc::PpcRuntime> ppc;
    fh2::input::InputState input;
    fh2::fs::FsProvider fs;
    ANativeWindow* window = nullptr;
    int width = 0, height = 0;
    std::thread mainThread;
    std::string filesDir;                   // setado antes do boot via JNI
};

RuntimeState& state() {
    static RuntimeState s;
    return s;
}

void gameMain(RuntimeState& s) {
    LOGI("gameMain: thread principal iniciada");
#if FH2_HAS_RECOMP
    // Fase atual do projeto: runtime inicializa subsistemas e aciona o guest.
    // A execução completa do guest exige a integração do kernel/IO (issue #16).
    s.ppc->run(s.gfx.get(), s.audio.get(), &s.input, &s.fs, s.stopRequested, s.paused);
#else
    // Modo shell: sem código recompilado, apenas sinaliza e aguarda stop.
    LOGI("Modo shell: código recompilado ausente (gere com tools/ ou confira o CI)");
    while (!s.stopRequested) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
#endif
    LOGI("gameMain: saída");
}

} // namespace

extern "C" {

JNIEXPORT void JNICALL
Java_com_fh2recomp_nativebridge_NativeBridge_nativeSetFilesDir(
        JNIEnv* env, jobject, jstring path) {
    auto& s = state();
    if (!path) return;
    const char* p = env->GetStringUTFChars(path, nullptr);
    if (!p) return;
    {
        std::lock_guard<std::mutex> lock(s.lifecycleMutex);
        s.filesDir = p;
        s.fs.setFilesDir(s.filesDir);
    }
    env->ReleaseStringUTFChars(path, p);
}

JNIEXPORT jboolean JNICALL
Java_com_fh2recomp_nativebridge_NativeBridge_nativeBoot(
        JNIEnv* env, jobject, jstring assetsTreeUri, jint resolutionScalePct,
        jboolean fps60, jboolean useVulkan) {
    auto& s = state();
    std::lock_guard<std::mutex> lock(s.lifecycleMutex);
    if (s.booted) return JNI_TRUE;

    const char* uri = assetsTreeUri ? env->GetStringUTFChars(assetsTreeUri, nullptr) : "";
    LOGI("nativeBoot: assets=%s scale=%d%% fps60=%d vulkan=%d",
         uri ? uri : "", resolutionScalePct, fps60 ? 1 : 0, useVulkan ? 1 : 0);

    s.fs.setAssetsTreeUri(uri ? uri : "");
    s.fs.initialize(env);
    s.gfx = fh2::gfx::createBackend(useVulkan != JNI_FALSE, resolutionScalePct);
    if (s.gfx) s.gfx->setFpsTarget(fps60 != JNI_FALSE ? 60 : 30);
    s.audio = fh2::audio::AudioOutput::create();
    s.ppc = std::make_unique<fh2::ppc::PpcRuntime>();
    bool ok = s.ppc->initialize(&s.fs);

    if (uri && assetsTreeUri) env->ReleaseStringUTFChars(assetsTreeUri, uri);

    if (!ok) {
        LOGE("nativeBoot: inicialização do guest falhou (memória/loader XEX — "
             "ver FH2/PPC e FH2/XEX no logcat)");
        s.ppc.reset();
        s.audio.reset();
        s.gfx.reset();
        return JNI_FALSE;
    }

    s.booted = true;
    s.paused = false;
    s.stopRequested = false;
    s.mainThread = std::thread(gameMain, std::ref(s));
    return JNI_TRUE;
}

JNIEXPORT void JNICALL
Java_com_fh2recomp_nativebridge_NativeBridge_nativeSetSurface(
        JNIEnv* env, jobject, jobject surface, jint width, jint height) {
    auto& s = state();
    std::lock_guard<std::mutex> lock(s.lifecycleMutex);
    ANativeWindow* win = surface ? ANativeWindow_fromSurface(env, surface) : nullptr;
    if (s.window) ANativeWindow_release(s.window);
    s.window = win;
    s.width = width;
    s.height = height;
    if (win && s.gfx) s.gfx->onSurfaceAvailable(win, width, height);
    LOGI("surface: %dx%d (%s)", width, height, win ? "set" : "released");
}

JNIEXPORT void JNICALL
Java_com_fh2recomp_nativebridge_NativeBridge_nativePause(JNIEnv*, jobject) {
    auto& s = state();
    if (!s.booted) return;
    s.paused = true;
    if (s.gfx) s.gfx->onSurfaceLost();   // perda de contexto ao minimizar
    if (s.audio) s.audio->pause();
    LOGI("pause");
}

JNIEXPORT void JNICALL
Java_com_fh2recomp_nativebridge_NativeBridge_nativeResume(JNIEnv*, jobject) {
    auto& s = state();
    if (!s.booted) return;
    s.paused = false;
    if (s.audio) s.audio->resume();
    LOGI("resume");
}

JNIEXPORT void JNICALL
Java_com_fh2recomp_nativebridge_NativeBridge_nativeStop(JNIEnv*, jobject) {
    auto& s = state();
    {
        std::lock_guard<std::mutex> lock(s.lifecycleMutex);
        if (!s.booted) return;
        s.stopRequested = true;
    }
    // Acorda todos os waits/delays HLE — as threads guest fazem unwind.
    fh2::kern::requestStop();
    // join fora do lock: a thread do jogo não pega o lifecycleMutex
    if (s.mainThread.joinable()) s.mainThread.join();
    std::lock_guard<std::mutex> lock(s.lifecycleMutex);
    // Se ainda há threads guest vivas (ex.: spin sem HLE), a memória guest
    // NÃO pode ser desmapeada — retém até o fim do processo (decisão real,
    // documentada em DECISIONS D24).
    if (s.ppc && s.ppc->guestStarted() && fh2::kern::joinAll(2000) > 0) {
        LOGE("stop: threads guest ainda vivas — memória guest retida "
             "(leak controlado, evita SIGSEGV)");
        s.ppc.release();
    }
    if (s.window) { ANativeWindow_release(s.window); s.window = nullptr; }
    s.gfx.reset();
    s.audio.reset();
    s.ppc.reset();
    s.booted = false;
    LOGI("stop");
}

// --- Input: HUD virtual (port 0) e gamepads físicos (port 0..3) ---

JNIEXPORT void JNICALL
Java_com_fh2recomp_nativebridge_NativeBridge_onVirtualButton(JNIEnv*, jclass, jint id, jboolean pressed) {
    state().input.setVirtualButton(static_cast<int>(id), pressed != JNI_FALSE);
}

JNIEXPORT void JNICALL
Java_com_fh2recomp_nativebridge_NativeBridge_onVirtualStick(JNIEnv*, jclass, jfloat x, jfloat y) {
    state().input.setVirtualStick(x, y);
}

JNIEXPORT void JNICALL
Java_com_fh2recomp_nativebridge_NativeBridge_onGamepadEvent(JNIEnv*, jclass, jint port, jint code, jboolean pressed) {
    state().input.setGamepadButton(static_cast<int>(port), static_cast<int>(code), pressed != JNI_FALSE);
}

JNIEXPORT void JNICALL
Java_com_fh2recomp_nativebridge_NativeBridge_onGamepadAxis(JNIEnv*, jclass, jint port, jint axis, jfloat value) {
    state().input.setGamepadAxis(static_cast<int>(port), static_cast<int>(axis), value);
}

} // extern "C"
