// gles3_backend.h
#pragma once

#include <atomic>
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

    EGLDisplay display_ = EGL_NO_DISPLAY;
    EGLContext context_ = EGL_NO_CONTEXT;
    EGLSurface surface_ = EGL_NO_SURFACE;
    EGLConfig config_ = nullptr;
    EGLint numConfigs_ = 0;
};

} // namespace fh2::gles3
