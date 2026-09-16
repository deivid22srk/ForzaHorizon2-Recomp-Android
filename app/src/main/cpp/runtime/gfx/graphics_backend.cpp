// graphics_backend.cpp — fábrica de backends gráficos
#include "graphics_backend.h"

#include <memory>
#include <mutex>
#include <utility>

#include "gles3_backend.h"

#if FH2_ENABLE_VULKAN
#include "vulkan_backend.h"
#endif

namespace fh2::gfx {

namespace {
// Backend ativo para o kernel (VdSwap). Registro acontece no boot (antes de
// qualquer thread guest) e a limpeza no stop (após o join) — leitura sem
// lock é segura em runtime; o mutex protege o par set/clear.
std::mutex g_backendMutex;
GraphicsBackend* g_activeBackend = nullptr;
} // namespace

void setActiveBackend(GraphicsBackend* backend) {
    std::lock_guard<std::mutex> lock(g_backendMutex);
    g_activeBackend = backend;
}

GraphicsBackend* activeBackend() {
    std::lock_guard<std::mutex> lock(g_backendMutex);
    return g_activeBackend;
}

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
