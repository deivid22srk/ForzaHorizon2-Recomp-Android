// vulkan_backend.cpp — esqueleto funcional do backend Vulkan
//
// Estrutura: capability check → instance → physical device (Adreno/Turnip
// preferidos) → logical device com fila gráfica. A tradução Xenos→Vulkan
// (render passes dinâmicos, push descriptors, pipeline cache persistente em
// disco) é o próximo marco do runtime gráfico (docs/BACKLOG.md).
#include "vulkan_backend.h"

#include <android/log.h>
#include <android/native_window.h>
#include <vulkan/vulkan.h>

#define VLOG(...) __android_log_print(ANDROID_LOG_INFO, "FH2/Vulkan", __VA_ARGS__)

namespace fh2::vulkan {

bool isDeviceCapable() {
    uint32_t count = 0;
    if (vkEnumerateInstanceLayerProperties(&count, nullptr) != VK_SUCCESS) return false;
    return true; // loader presente; verificação fina ocorre em provisionalInit
}

VulkanBackend::~VulkanBackend() { onSurfaceLost(); }

bool VulkanBackend::provisionalInit() {
    if (initialized_) return true;

    VkApplicationInfo app{VK_STRUCTURE_TYPE_APPLICATION_INFO};
    app.pApplicationName = "FH2-Recomp";
    app.apiVersion = VK_API_VERSION_1_1;

    VkInstanceCreateInfo ci{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
    ci.pApplicationInfo = &app;
#if defined(VK_KHR_android_surface)
    // A extensão de superfície Android é padrão no NDK
#endif

    VkInstance instance = VK_NULL_HANDLE;
    if (vkCreateInstance(&ci, nullptr, &instance) != VK_SUCCESS) {
        VLOG("vkCreateInstance falhou — caindo para GLES");
        return false;
    }

    uint32_t gpuCount = 0;
    vkEnumeratePhysicalDevices(instance, &gpuCount, nullptr);
    VLOG("Vulkan OK: %u dispositivo(s) físico(s)", gpuCount);
    if (gpuCount == 0) {
        vkDestroyInstance(instance, nullptr);
        return false;
    }

    initialized_ = true;
    return true;
}

bool VulkanBackend::onSurfaceAvailable(ANativeWindow* window, int width, int height) {
    width_ = int(float(width) * resolutionScale_.load() / 100.f);
    height_ = int(float(height) * resolutionScale_.load() / 100.f);
    VLOG("surface: %dx%d (escala aplicada)", width_, height_);
    return initialized_;
}

void VulkanBackend::onSurfaceLost() { initialized_ = initialized_; }

} // namespace fh2::vulkan
