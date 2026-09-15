// native_main.cpp — ponte JNI do runtime FH2-Recomp (Android, mobile-first)
//
// Responsabilidades:
//  - Ciclo de vida (boot/pause/resume/stop/superfície) vindo da GameActivity
//  - Fila de input (HUD touch + gamepad físico) consumida pelo guest
//  - Bootstrap do PPC runtime e da thread principal do jogo recompilado
//
// O jogo recompilado só existe se recomp/generated estiver presente
// (FH2_HAS_RECOMP=1). Sem ele, o app opera em modo shell (setup do usuário).

#include <android/log.h>
#include <android/native_window.h>
#include <android/native_window_jni.h>
#include <jni.h>
#include <atomic>
#include <memory>
#include <string>
#include <thread>

#include "runtime/gfx/graphics_backend.h"
#include "runtime/audio/audio_output.h"
#include "runtime/input/input_state.h"
#include "runtime/fs/fs_provider.h"
#include "runtime/ppc/ppc_runtime.h"

#define LOG_TAG "FH2Recomp"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

namespace {

struct RuntimeState {
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
};

RuntimeState& state() {
    static RuntimeState s;
    return s;
}

void gameMain(RuntimeState& s) {
    LOGI("gameMain: thread principal iniciada");
#if FH2_HAS_RECOMP
    // Fase atual do projeto: runtime inicializa subsistemas e aciona o guest.
    // A execução completa do guest exige a integração do kernel/IO (backlog).
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

JNIEXPORT jboolean JNICALL
Java_com_fh2recomp_nativebridge_NativeBridge_nativeBoot(
        JNIEnv* env, jobject, jstring assetsTreeUri, jint resolutionScalePct,
        jboolean fps60, jboolean useVulkan) {
    auto& s = state();
    if (s.booted) return JNI_TRUE;

    const char* uri = assetsTreeUri ? env->GetStringUTFChars(assetsTreeUri, nullptr) : "";
    LOGI("nativeBoot: assets=%s scale=%d%% fps60=%d vulkan=%d",
         uri, resolutionScalePct, fps60 ? 1 : 0, useVulkan ? 1 : 0);

    s.fs.initialize(env, uri);
    s.gfx = fh2::gfx::createBackend(useVulkan != JNI_FALSE, resolutionScalePct);
    s.audio = fh2::audio::AudioOutput::create();
    s.ppc = std::make_unique<fh2::ppc::PpcRuntime>();
    s.ppc->initialize(&s.fs);

    s.booted = true;
    s.paused = false;
    s.stopRequested = false;
    s.mainThread = std::thread(gameMain, std::ref(s));
    if (uri && assetsTreeUri) env->ReleaseStringUTFChars(assetsTreeUri, uri);
    return JNI_TRUE;
}

JNIEXPORT void JNICALL
Java_com_fh2recomp_nativebridge_NativeBridge_nativeSetSurface(
        JNIEnv* env, jobject, jobject surface, jint width, jint height) {
    auto& s = state();
    if (!s.booted) return;
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
    if (!s.booted) return;
    s.stopRequested = true;
    if (s.mainThread.joinable()) s.mainThread.join();
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
