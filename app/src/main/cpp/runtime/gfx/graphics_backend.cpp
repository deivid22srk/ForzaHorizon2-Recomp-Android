// graphics_backend.cpp — fábrica de backends gráficos
#include "graphics_backend.h"

#include <memory>
#include <utility>

#include "gles3_backend.h"

#if FH2_ENABLE_VULKAN
#include "vulkan_backend.h"
#endif

namespace fh2::gfx {

std::unique_ptr<GraphicsBackend> createBackend(bool preferVulkan, int resolutionScalePct) {
#if FH2_ENABLE_VULKAN
    if (preferVulkan && vulkan::isDeviceCapable()) {
        auto vk = std::make_unique<vulkan::VulkanBackend>();
        if (vk->provisionalInit()) {
            vk->setResolutionScale(resolutionScalePct);
            return vk;
        }
    }
#endif
    auto gl = std::make_unique<gles3::Gles3Backend>();
    gl->setResolutionScale(resolutionScalePct);
    return gl;
}

} // namespace fh2::gfx
