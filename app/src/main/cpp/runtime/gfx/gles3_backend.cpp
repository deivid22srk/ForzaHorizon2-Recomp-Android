// gles3_backend.cpp — backend GLES 3.1 (EGL)
#include "gles3_backend.h"

#include <android/log.h>
#include <android/native_window.h>
#include <EGL/egl.h>
#include <EGL/eglplatform.h>
#include <GLES3/gl31.h>

#define GLOG(...) __android_log_print(ANDROID_LOG_INFO, "FH2/GLES", __VA_ARGS__)

namespace fh2::gles3 {

Gles3Backend::~Gles3Backend() { onSurfaceLost(); }

bool Gles3Backend::makeContext(ANativeWindow* window) {
    EGLDisplay display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (display == EGL_NO_DISPLAY) return false;
    if (!eglInitialize(display, nullptr, nullptr)) return false;

    const EGLint configAttribs[] = {
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT,
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_RED_SIZE, 8, EGL_GREEN_SIZE, 8, EGL_BLUE_SIZE, 8,
        EGL_ALPHA_SIZE, 8, EGL_DEPTH_SIZE, 24, EGL_STENCIL_SIZE, 8,
        EGL_NONE};
    EGLConfig config;
    EGLint numConfigs = 0;
    if (!eglChooseConfig(display, configAttribs, &config, 1, &numConfigs) || numConfigs < 1) {
        GLOG("eglChooseConfig falhou");
        return false;
    }

    const EGLint surfaceAttribs[] = {EGL_NONE};
    EGLSurface surface = eglCreateWindowSurface(display, config, window, surfaceAttribs);
    if (surface == EGL_NO_SURFACE) {
        GLOG("eglCreateWindowSurface falhou: 0x%x", eglGetError());
        return false;
    }

    const EGLint contextAttribs[] = {EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE};
    EGLContext context = eglCreateContext(display, config, EGL_NO_CONTEXT, contextAttribs);
    if (context == EGL_NO_CONTEXT) {
        GLOG("eglCreateContext falhou: 0x%x", eglGetError());
        return false;
    }

    if (!eglMakeCurrent(display, surface, surface, context)) {
        GLOG("eglMakeCurrent falhou: 0x%x", eglGetError());
        return false;
    }

    contextAlive_ = true;
    GLOG("contexto GLES 3.1 criado: %s / %s", glGetString(GL_RENDERER), glGetString(GL_VERSION));
    return true;
}

bool Gles3Backend::onSurfaceAvailable(ANativeWindow* window, int width, int height) {
    width_ = int(float(width) * resolutionScale_.load() / 100.f);
    height_ = int(float(height) * resolutionScale_.load() / 100.f);
    if (!contextAlive_) return makeContext(window);
    // Superfície recriada: realoca buffers (contexto preservado quando possível)
    GLOG("surface realloc: %dx%d (escala aplicada)", width_, height_);
    return true;
}

void Gles3Backend::onSurfaceLost() {
    if (contextAlive_) {
        EGLDisplay display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
        eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        contextAlive_ = false;
        GLOG("surface perdida: contexto liberado");
    }
}

} // namespace fh2::gles3
