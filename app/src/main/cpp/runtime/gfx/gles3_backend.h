// gles3_backend.h
#pragma once

#include <atomic>
#include <chrono>
#include <mutex>
#include <EGL/egl.h>

#include "graphics_backend.h"

struct ANativeWindow;

namespace fh2::gles3 {

/**
 * Backend OpenGL ES 3.1 — fallback universal em Android.
 *
 * Estado atual (v0.1): contexto EGL + superfície com ciclo de vida completo
 * (criação/destruição por superfície, sem leaks entre pause/resume). A
 * tradução das chamadas D3D9/Xenos é o próximo marco do runtime gráfico
 * (docs/BACKLOG.md — issue #17).
 */
class Gles3Backend : public gfx::GraphicsBackend {
public:
    ~Gles3Backend() override;

    bool onSurfaceAvailable(ANativeWindow* window, int width, int height) override;
    void onSurfaceLost() override;
    bool present() override;
    bool presentFrontBuffer(const void* data, uint32_t rowBytes, uint32_t w,
                            uint32_t h, uint32_t xenosFormat) override;

    void setResolutionScale(int pct) override { resolutionScale_ = pct; }
    void setFpsTarget(int fps) override { fpsTarget_ = fps; }
    int fpsTarget() const override { return fpsTarget_.load(); }

    const char* name() const override { return "GLES 3.1"; }
    gfx::SurfaceInfo surfaceInfo() const override { return {uint32_t(width_), uint32_t(height_)}; }

private:
    bool ensureDisplay();
    bool ensureContext();
    bool makeWindowSurface(ANativeWindow* window);

    std::atomic<int> resolutionScale_{75};
    std::atomic<int> fpsTarget_{60};
    int width_ = 0;
    int height_ = 0;
    bool contextAlive_ = false;
    std::mutex eglMutex_;                 // present (guest) × surface (UI thread)
    uint64_t frameCounter_ = 0;
    uint64_t lastFpsLog_ = 0;
    std::chrono::steady_clock::time_point lastPresentLog_{};

    EGLDisplay display_ = EGL_NO_DISPLAY;
    EGLContext context_ = EGL_NO_CONTEXT;
    EGLSurface surface_ = EGL_NO_SURFACE;
    EGLConfig config_ = nullptr;
    EGLint numConfigs_ = 0;

    // Front buffer REAL do título (upload + quad — pixels do jogo, p/ swap).
    unsigned fbTex_ = 0;                  // GLuint
    unsigned fbProgram_ = 0;              // GLuint
    int fbWidth_ = 0;
    int fbHeight_ = 0;
    bool fbShaderReady_ = false;
    bool ensureFrontBufferShader();
};

} // namespace fh2::gles3
