// vulkan_backend.h — backend Vulkan (prioritário; fallback GLES quando indisponível)
#pragma once

#include <atomic>
#include <vulkan/vulkan.h>

#include "graphics_backend.h"

namespace fh2::vulkan {

/** Verificação de capacidade do device: loader presente + versão mínima 1.1. */
bool isDeviceCapable();

class VulkanBackend : public gfx::GraphicsBackend {
public:
    ~VulkanBackend() override;

    /** Instancia instance/device provisionais; falha => runtime cai para GLES. */
    bool provisionalInit();

    bool onSurfaceAvailable(ANativeWindow* window, int width, int height) override;
    void onSurfaceLost() override;

    void setResolutionScale(int pct) override { resolutionScale_ = pct; }
    void setFpsTarget(int fps) override { fpsTarget_ = fps; }
    int fpsTarget() const override { return fpsTarget_.load(); }

    const char* name() const override { return "Vulkan"; }
    gfx::SurfaceInfo surfaceInfo() const override { return {uint32_t(width_), uint32_t(height_)}; }

private:
    std::atomic<int> resolutionScale_{75};
    std::atomic<int> fpsTarget_{60};
    int width_ = 0;
    int height_ = 0;
    bool initialized_ = false;
    VkInstance instance_ = VK_NULL_HANDLE;
};

} // namespace fh2::vulkan
