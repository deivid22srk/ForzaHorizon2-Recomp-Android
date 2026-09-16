// gles3_backend.cpp — backend GLES 3.1 (EGL)
//
// Ciclo de vida dos recursos: display/context são reutilizados entre
// superfícies; a EGLSurface é destruída e recriada para cada ANativeWindow
// (superfícies Android não são reutilizáveis). Toda destruição acontece em
// onSurfaceLost()/destructor — sem leaks entre pause/resume.
#include "gles3_backend.h"

#include <android/log.h>
#include <android/native_window.h>
#include <EGL/egl.h>
#include <EGL/eglplatform.h>
#include <GLES3/gl31.h>

#include <chrono>
#include <cmath>
#include <mutex>

#define GLOG(...) __android_log_print(ANDROID_LOG_INFO, "FH2/GLES", __VA_ARGS__)
#define GERR(...) __android_log_print(ANDROID_LOG_ERROR, "FH2/GLES", __VA_ARGS__)

namespace fh2::gles3 {

Gles3Backend::~Gles3Backend() { onSurfaceLost(); }

bool Gles3Backend::ensureDisplay() {
    if (display_ == EGL_NO_DISPLAY) {
        display_ = eglGetDisplay(EGL_DEFAULT_DISPLAY);
        if (display_ == EGL_NO_DISPLAY) return false;
        if (!eglInitialize(display_, nullptr, nullptr)) {
            display_ = EGL_NO_DISPLAY;
            return false;
        }
    }
    return true;
}

bool Gles3Backend::ensureContext() {
    if (context_ != EGL_NO_CONTEXT) return true;
    if (!ensureDisplay()) return false;

    const EGLint configAttribs[] = {
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT,
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_RED_SIZE, 8, EGL_GREEN_SIZE, 8, EGL_BLUE_SIZE, 8,
        EGL_ALPHA_SIZE, 8, EGL_DEPTH_SIZE, 24, EGL_STENCIL_SIZE, 8,
        EGL_NONE};
    if (!eglChooseConfig(display_, configAttribs, &config_, 1, &numConfigs_) || numConfigs_ < 1) {
        GLOG("eglChooseConfig falhou");
        return false;
    }

    const EGLint contextAttribs[] = {EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE};
    context_ = eglCreateContext(display_, config_, EGL_NO_CONTEXT, contextAttribs);
    if (context_ == EGL_NO_CONTEXT) {
        GLOG("eglCreateContext falhou: 0x%x", eglGetError());
        return false;
    }
    return true;
}

bool Gles3Backend::makeWindowSurface(ANativeWindow* window) {
    if (!ensureDisplay() || !ensureContext()) return false;

    // Superfície anterior pertence a uma ANativeWindow morta — destruir sempre.
    if (surface_ != EGL_NO_SURFACE) {
        eglDestroySurface(display_, surface_);
        surface_ = EGL_NO_SURFACE;
    }

    const EGLint surfaceAttribs[] = {EGL_NONE};
    surface_ = eglCreateWindowSurface(display_, config_, window, surfaceAttribs);
    if (surface_ == EGL_NO_SURFACE) {
        GLOG("eglCreateWindowSurface falhou: 0x%x", eglGetError());
        return false;
    }
    if (!eglMakeCurrent(display_, surface_, surface_, context_)) {
        GLOG("eglMakeCurrent falhou: 0x%x", eglGetError());
        return false;
    }
    contextAlive_ = true;
    GLOG("superfície GLES 3.1 ativa: %s / %s", glGetString(GL_RENDERER), glGetString(GL_VERSION));
    return true;
}

bool Gles3Backend::onSurfaceAvailable(ANativeWindow* window, int width, int height) {
    std::lock_guard<std::mutex> lock(eglMutex_);
    width_ = int(float(width) * resolutionScale_.load() / 100.f);
    height_ = int(float(height) * resolutionScale_.load() / 100.f);
    return makeWindowSurface(window);
}

void Gles3Backend::onSurfaceLost() {
    std::lock_guard<std::mutex> lock(eglMutex_);
    if (display_ != EGL_NO_DISPLAY) {
        eglMakeCurrent(display_, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        if (surface_ != EGL_NO_SURFACE) {
            eglDestroySurface(display_, surface_);
            surface_ = EGL_NO_SURFACE;
        }
        if (context_ != EGL_NO_CONTEXT) {
            eglDestroyContext(display_, context_);
            context_ = EGL_NO_CONTEXT;
        }
        eglTerminate(display_);
        display_ = EGL_NO_DISPLAY;
    }
    config_ = nullptr;
    numConfigs_ = 0;
    contextAlive_ = false;
    GLOG("surface perdida: recursos EGL liberados");
}

bool Gles3Backend::present() {
    std::lock_guard<std::mutex> lock(eglMutex_);
    if (!contextAlive_ || display_ == EGL_NO_DISPLAY || surface_ == EGL_NO_SURFACE) {
        return false;
    }
    // Contexto EGL é por-thread: a thread chamadora (guest via VdSwap ou o
    // driver de frames) adota o contexto — EGL permite mover entre threads.
    if (eglGetCurrentContext() != context_) {
        if (!eglMakeCurrent(display_, surface_, surface_, context_)) {
            GERR("eglMakeCurrent (present) falhou: 0x%x", eglGetError());
            return false;
        }
    }
    // Padrão de teste REAL do pipeline (cor animada por frameCounter_) —
    // substituído pelos pixels do Xenos na issue #17.
    const float t = float(frameCounter_ % 720) / 720.0f;
    const float h = t * 6.0f;
    const float v = 0.45f + 0.10f * std::sin(float(frameCounter_) * 0.05f);
    const int i = int(h) % 6;
    const float f = h - std::floor(h);
    const float s = 0.85f;
    const float p = v * (1.0f - s);
    const float q = v * (1.0f - s * f);
    const float t2 = v * (1.0f - s * (1.0f - f));
    float r = 0, g = 0, b = 0;
    switch (i) {
        case 0: r = v; g = t2; b = p; break;
        case 1: r = q; g = v; b = p; break;
        case 2: r = p; g = v; b = t2; break;
        case 3: r = p; g = q; b = v; break;
        case 4: r = t2; g = p; b = v; break;
        default: r = v; g = p; b = q; break;
    }
    glClearColor(r, g, b, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    if (!eglSwapBuffers(display_, surface_)) {
        const EGLint err = eglGetError();
        GERR("eglSwapBuffers falhou: 0x%x", err);
        if (err == EGL_CONTEXT_LOST || err == EGL_BAD_NATIVE_WINDOW) {
            contextAlive_ = false;
        }
        return false;
    }
    ++frameCounter_;
    if (frameCounter_ - lastFpsLog_ >= 300) {
        static auto last = std::chrono::steady_clock::now();
        auto now = std::chrono::steady_clock::now();
        double dt = std::chrono::duration<double>(now - last).count();
        last = now;
        GLOG("present: %llu frames (%.1f fps medidos)",
             (unsigned long long)frameCounter_, dt > 0 ? 300.0 / dt : 0.0);
        lastFpsLog_ = frameCounter_;
    }
    return true;
}

} // namespace fh2::gles3
