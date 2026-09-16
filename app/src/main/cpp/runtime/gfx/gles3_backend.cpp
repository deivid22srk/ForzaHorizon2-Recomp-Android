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
    // Recursos GL morreram com o contexto — força recriação limpa.
    fbTex_ = 0;
    fbProgram_ = 0;
    fbWidth_ = fbHeight_ = 0;
    fbShaderReady_ = false;
    GLOG("surface perdida: recursos EGL liberados");
}

bool Gles3Backend::ensureFrontBufferShader() {
    if (fbShaderReady_ && fbProgram_ != 0) return true;
    // Fullscreen quad: amostra a textura do front buffer REAL do título.
    const char* vs = R"(#version 310 es
        precision highp float;
        out vec2 vUV;
        void main() {
            vec2 p = vec2(float((gl_VertexID << 1) & 2), float(gl_VertexID & 2));
            vUV = p;
            gl_Position = vec4(p * 2.0 - 1.0, 0.0, 1.0);
        })";
    const char* fs = R"(#version 310 es
        precision highp float;
        in vec2 vUV;
        uniform sampler2D uTex;
        out vec4 fragColor;
        void main() { fragColor = texture(uTex, vUV); })";
    auto compile = [](GLenum type, const char* src) -> unsigned {
        unsigned sh = glCreateShader(type);
        glShaderSource(sh, 1, &src, nullptr);
        glCompileShader(sh);
        GLint ok = 0;
        glGetShaderiv(sh, GL_COMPILE_STATUS, &ok);
        if (!ok) {
            glDeleteShader(sh);
            return 0;
        }
        return sh;
    };
    unsigned v = compile(GL_VERTEX_SHADER, vs);
    unsigned fr = compile(GL_FRAGMENT_SHADER, fs);
    if (!v || !fr) {
        if (v) glDeleteShader(v);
        if (fr) glDeleteShader(fr);
        GERR("front buffer: compilação do shader falhou");
        return false;
    }
    fbProgram_ = glCreateProgram();
    glAttachShader(fbProgram_, v);
    glAttachShader(fbProgram_, fr);
    glLinkProgram(fbProgram_);
    glDeleteShader(v);
    glDeleteShader(fr);
    GLint ok = 0;
    glGetProgramiv(fbProgram_, GL_LINK_STATUS, &ok);
    if (!ok) {
        glDeleteProgram(fbProgram_);
        fbProgram_ = 0;
        GERR("front buffer: link do shader falhou");
        return false;
    }
    fbShaderReady_ = true;
    return true;
}

bool Gles3Backend::presentFrontBuffer(const void* data, uint32_t rowBytes,
                                      uint32_t w, uint32_t h,
                                      uint32_t xenosFormat) {
    // Só k_8_8_8_8 (6) neste marco (o CP não chama para os outros).
    if (xenosFormat != 6 || data == nullptr) return false;
    std::lock_guard<std::mutex> lock(eglMutex_);
    if (!contextAlive_ || display_ == EGL_NO_DISPLAY ||
        surface_ == EGL_NO_SURFACE) {
        return false;
    }
    if (eglGetCurrentContext() != context_) {
        if (!eglMakeCurrent(display_, surface_, surface_, context_)) return false;
    }
    if (!ensureFrontBufferShader()) return false;
    if (fbTex_ == 0) glGenTextures(1, &fbTex_);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, fbTex_);
    if (fbWidth_ != int(w) || fbHeight_ != int(h)) {
        // rowBytes pode diferir de w*4 (pitch do guest): UNPACK_ROW_LENGTH.
        glPixelStorei(GL_UNPACK_ROW_LENGTH, rowBytes / 4);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA,
                     GL_UNSIGNED_BYTE, data);
        glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        fbWidth_ = int(w);
        fbHeight_ = int(h);
    } else {
        glPixelStorei(GL_UNPACK_ROW_LENGTH, rowBytes / 4);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, w, h, GL_RGBA,
                        GL_UNSIGNED_BYTE, data);
        glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    }

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    glUseProgram(fbProgram_);
    glUniform1i(glGetUniformLocation(fbProgram_, "uTex"), 0);
    glViewport(0, 0, width_, height_);
    glDrawArrays(GL_TRIANGLES, 0, 3); // fullscreen triangle

    if (!eglSwapBuffers(display_, surface_)) {
        const EGLint err = eglGetError();
        GERR("eglSwapBuffers falhou: 0x%x", err);
        if (err == EGL_CONTEXT_LOST || err == EGL_BAD_NATIVE_WINDOW) {
            contextAlive_ = false;
        }
        return false;
    }
    ++frameCounter_;
    return true;
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
    // Clear PRETO — o backend nunca inventa conteúdo: enquanto o Xenos
    // (issue #17) não produzir o frame real do título, a surface é preta.
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
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
        auto now = std::chrono::steady_clock::now();
        if (lastPresentLog_ != std::chrono::steady_clock::time_point{}) {
            double dt = std::chrono::duration<double>(now - lastPresentLog_).count();
            if (dt > 0.5) {
                GLOG("present: %llu frames (%.1f fps medidos)",
                     (unsigned long long)frameCounter_, 300.0 / dt);
            }
        }
        lastPresentLog_ = now;
        lastFpsLog_ = frameCounter_;
    }
    return true;
}

} // namespace fh2::gles3
