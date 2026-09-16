// vulkan_backend.h — backend Vulkan REAL (prioritário; fallback GLES)
//
// Pipeline completo: instance → physical device → fila gráfica → device
// lógico → VkSurfaceKHR (Android) → swapchain (FIFO = vblank) → render
// pass → framebuffers → command buffers → sync (semáforos + fences) →
// present.
//
// POLÍTICA DE CONTEÚDO: este backend NUNCA inventa pixels. Enquanto o
// processador de comandos Xenos (issue #17) não produzir o frame real do
// título, todo present é um clear PRETO — uma surface colorida aqui
// fingiria que o jogo está rodando. Os pixels do jogo chegam quando o
// processador de comandos estiver consumindo o ring buffer real do guest
// (o backend já entrega tudo que ele precisa: swapchain recriável, fila,
// present bloqueante = vblank).
#pragma once

#include <atomic>
#include <chrono>
#include <cstdint>
#include <mutex>
#include <vector>

// Plataforma Android p/ o header do NDK (superfície VK_KHR_android_surface).
// Precisa estar definido ANTES de <vulkan/vulkan.h>.
#ifndef VK_USE_PLATFORM_ANDROID_KHR
#define VK_USE_PLATFORM_ANDROID_KHR 1
#endif

#include <vulkan/vulkan.h>

#include "graphics_backend.h"

struct ANativeWindow;

namespace fh2::vulkan {

/** Capacidade REAL do device: loader Vulkan presente + extensions de
 *  superfície Android + ao menos um physical device com fila gráfica. */
bool isDeviceCapable();

class VulkanBackend final : public gfx::GraphicsBackend {
public:
    ~VulkanBackend() override;

    /** Instancia instance/device provisionais; falha => runtime cai p/ GLES. */
    bool provisionalInit();

    bool onSurfaceAvailable(ANativeWindow* window, int width, int height) override;
    void onSurfaceLost() override;
    bool present() override;

    void setResolutionScale(int pct) override { resolutionScale_ = pct; }
    void setFpsTarget(int fps) override { fpsTarget_ = fps; }
    int fpsTarget() const override { return fpsTarget_.load(); }

    const char* name() const override { return "Vulkan"; }
    gfx::SurfaceInfo surfaceInfo() const override { return {uint32_t(width_), uint32_t(height_)}; }

private:
    // fases do init (idempotentes; chamadas por onSurfaceAvailable/provisionalInit)
    bool ensureInstance();
    bool ensureDevice();
    bool ensureSurface(ANativeWindow* window);
    bool createSwapchainObjects();   // swapchain + views + renderpass + fb + cmds + sync
    void destroySwapchainObjects();
    bool recreateSwapchain();

    // destruição total (device + instance) — onSurfaceLost/destructor
    void destroyDeviceAndInstance();

    // Recria um fence do slot como SIGNALED (recuperação de erro de submit —
    // sem isso o próximo vkWaitForFences do slot bloquearia para sempre).
    void recreateFenceSignaled(uint32_t slot);

    // padrão de teste REMOVIDO — o backend nunca inventa conteúdo: enquanto
    // o Xenos não produzir frames reais a surface é preta
    std::chrono::steady_clock::time_point lastPresentLog_{};

    std::mutex gpuMutex_;                 // present × surface × destruição
    std::atomic<int> resolutionScale_{75};
    std::atomic<int> fpsTarget_{60};
    int width_ = 0;
    int height_ = 0;
    bool instanceReady_ = false;
    bool deviceReady_ = false;

    ANativeWindow* window_ = nullptr;     // referência fraca (dono é o runtime)
    VkInstance instance_ = VK_NULL_HANDLE;
    VkPhysicalDevice pdev_ = VK_NULL_HANDLE;
    VkDevice device_ = VK_NULL_HANDLE;
    VkQueue queue_ = VK_NULL_HANDLE;      // família gráfica (present incluído)
    uint32_t queueFamily_ = 0;

    VkSurfaceKHR surface_ = VK_NULL_HANDLE;
    VkSwapchainKHR swapchain_ = VK_NULL_HANDLE;
    VkFormat swapFormat_ = VK_FORMAT_B8G8R8A8_UNORM;
    VkExtent2D extent_{0, 0};
    std::vector<VkImage> images_;
    std::vector<VkImageView> views_;

    VkRenderPass renderPass_ = VK_NULL_HANDLE;
    std::vector<VkFramebuffer> framebuffers_;

    VkCommandPool cmdPool_ = VK_NULL_HANDLE;
    std::vector<VkCommandBuffer> cmds_;   // 1 por imagem do swapchain

    // sincronização POR FRAME EM VOO (modelo canônico: no máximo 2 frames
    // pendentes; o fence do slot garante que seus semáforos estão livres)
    static constexpr uint32_t kMaxInFlight = 2;
    std::vector<VkSemaphore> imageAvailable_;
    std::vector<VkSemaphore> renderFinished_;
    std::vector<VkFence> inFlight_;

    uint64_t frameCounter_ = 0;           // frames apresentados de verdade
    uint64_t lastFpsLog_ = 0;
    bool surfaceValid_ = false;
};

} // namespace fh2::vulkan
