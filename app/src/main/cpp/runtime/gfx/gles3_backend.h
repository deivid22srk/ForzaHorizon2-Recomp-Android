// gles3_backend.h
#pragma once

#include <atomic>

#include "graphics_backend.h"

struct ANativeWindow;

namespace fh2::gles3 {

/**
 * Backend OpenGL ES 3.1 — fallback universal em Android.
 *
 * Estado atual (v0.1): contexto EGL + clear/frame loop funcionais, cache de
 * programas e pipeline de texturas prontos para receber a tradução das
 * chamadas D3D9/Xenos (backlog do runtime gráfico).
 */
class Gles3Backend : public gfx::GraphicsBackend {
public:
    ~Gles3Backend() override;

    bool onSurfaceAvailable(ANativeWindow* window, int width, int height) override;
    void onSurfaceLost() override;

    void setResolutionScale(int pct) override { resolutionScale_ = pct; }
    void setFpsTarget(int fps) override { fpsTarget_ = fps; }

    const char* name() const override { return "GLES 3.1"; }
    gfx::SurfaceInfo surfaceInfo() const override { return {uint32_t(width_), uint32_t(height_)}; }

private:
    bool makeContext(ANativeWindow* window);

    std::atomic<int> resolutionScale_{75};
    std::atomic<int> fpsTarget_{60};
    int width_ = 0;
    int height_ = 0;
    bool contextAlive_ = false;
};

} // namespace fh2::gles3
