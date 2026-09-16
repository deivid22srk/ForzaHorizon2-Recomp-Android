// vulkan_backend.cpp — backend Vulkan REAL do FH2-Recomp
//
// Implementação completa do marco "renderizar algo em Vulkan":
//   1. instance com extensions de superfície Android (checadas de verdade)
//   2. physical device (prefere Adreno/discreto) + família gráfica
//   3. device lógico + fila
//   4. VkSurfaceKHR via VK_KHR_android_surface
//   5. swapchain FIFO (vblank real do display), formato 8-bit escolhido
//      das superfícies suportadas, pretransform atual (sem rotação extra)
//   6. render pass (clear+store) + framebuffers por imagem
//   7. command buffer por imagem + semáforos/fences por imagem
//   8. present(): acquire → clear real → submit → present (bloqueia =
//      vblank; mesma semântica do VdSwap do console)
//
// Conteúdo do frame: PADRÃO DE TESTE do pipeline (cor animada real via
// vkCmdClearAttachments) — não são pixels do jogo; eles chegam com o
// processador de comandos Xenos (issue #17). Toda falha de API é logada
// com o VkResult e tratada (recreate/teardown) — sem sucesso falso.
#include "vulkan_backend.h"

// Exposta somente com o define de plataforma (header do NDK) — a superfície
// Android (VK_KHR_android_surface + vkCreateAndroidSurfaceKHR) sai do modo
// "declared" com isto antes do <vulkan/vulkan.h> (que o header inclui).
#ifndef VK_USE_PLATFORM_ANDROID_KHR
#define VK_USE_PLATFORM_ANDROID_KHR 1
#endif

#include <android/log.h>
#include <android/native_window.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstring>

#define VLOG(...) __android_log_print(ANDROID_LOG_INFO, "FH2/Vulkan", __VA_ARGS__)
#define VERR(...) __android_log_print(ANDROID_LOG_ERROR, "FH2/Vulkan", __VA_ARGS__)

namespace fh2::vulkan {

namespace {

const char* vkResultStr(VkResult r) {
    switch (r) {
        case VK_SUCCESS: return "VK_SUCCESS";
        case VK_ERROR_OUT_OF_DATE_KHR: return "VK_ERROR_OUT_OF_DATE_KHR";
        case VK_SUBOPTIMAL_KHR: return "VK_SUBOPTIMAL_KHR";
        case VK_ERROR_SURFACE_LOST_KHR: return "VK_ERROR_SURFACE_LOST_KHR";
        case VK_ERROR_INITIALIZATION_FAILED: return "VK_ERROR_INITIALIZATION_FAILED";
        case VK_ERROR_DEVICE_LOST: return "VK_ERROR_DEVICE_LOST";
        case VK_ERROR_OUT_OF_HOST_MEMORY: return "VK_ERROR_OUT_OF_HOST_MEMORY";
        case VK_ERROR_OUT_OF_DEVICE_MEMORY: return "VK_ERROR_OUT_OF_DEVICE_MEMORY";
        default: return "VK_ERROR_*";
    }
}

} // namespace

// ----------------------------------------------------------- capacidade

bool isDeviceCapable() {
    // Sonda REAL: extensions de instância Android disponíveis + ≥1 physical
    // device com fila gráfica. Destrói tudo antes de retornar (sem efeito).
    uint32_t extCount = 0;
    if (vkEnumerateInstanceExtensionProperties(nullptr, &extCount, nullptr) != VK_SUCCESS ||
        extCount == 0) {
        return false;
    }
    std::vector<VkExtensionProperties> exts(extCount);
    if (vkEnumerateInstanceExtensionProperties(nullptr, &extCount, exts.data()) != VK_SUCCESS) {
        return false;
    }
    bool hasSurface = false, hasAndroidSurface = false;
    for (const auto& e : exts) {
        if (strcmp(e.extensionName, VK_KHR_SURFACE_EXTENSION_NAME) == 0) hasSurface = true;
        if (strcmp(e.extensionName, VK_KHR_ANDROID_SURFACE_EXTENSION_NAME) == 0)
            hasAndroidSurface = true;
    }
    if (!hasSurface || !hasAndroidSurface) return false;

    VkApplicationInfo app{VK_STRUCTURE_TYPE_APPLICATION_INFO};
    app.pApplicationName = "FH2-Recomp";
    app.apiVersion = VK_API_VERSION_1_1;
    VkInstanceCreateInfo ci{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
    ci.pApplicationInfo = &app;
    VkInstance probe = VK_NULL_HANDLE;
    if (vkCreateInstance(&ci, nullptr, &probe) != VK_SUCCESS) return false;

    uint32_t gpuCount = 0;
    bool capable = false;
    if (vkEnumeratePhysicalDevices(probe, &gpuCount, nullptr) == VK_SUCCESS && gpuCount > 0) {
        std::vector<VkPhysicalDevice> gpus(gpuCount);
        if (vkEnumeratePhysicalDevices(probe, &gpuCount, gpus.data()) == VK_SUCCESS) {
            for (VkPhysicalDevice gpu : gpus) {
                uint32_t famCount = 0;
                vkGetPhysicalDeviceQueueFamilyProperties(gpu, &famCount, nullptr);
                std::vector<VkQueueFamilyProperties> fams(famCount);
                vkGetPhysicalDeviceQueueFamilyProperties(gpu, &famCount, fams.data());
                for (const auto& f : fams) {
                    if (f.queueFlags & VK_QUEUE_GRAPHICS_BIT) { capable = true; break; }
                }
                if (capable) break;
            }
        }
    }
    vkDestroyInstance(probe, nullptr);
    return capable;
}

// ------------------------------------------------------- ciclo de vida

VulkanBackend::~VulkanBackend() { onSurfaceLost(); }

bool VulkanBackend::ensureInstance() {
    if (instanceReady_) return true;

    uint32_t extCount = 0;
    if (vkEnumerateInstanceExtensionProperties(nullptr, &extCount, nullptr) != VK_SUCCESS) {
        VERR("vkEnumerateInstanceExtensionProperties falhou — sem loader Vulkan");
        return false;
    }
    std::vector<VkExtensionProperties> exts(extCount);
    vkEnumerateInstanceExtensionProperties(nullptr, &extCount, exts.data());
    bool hasSurface = false, hasAndroidSurface = false;
    for (const auto& e : exts) {
        if (strcmp(e.extensionName, VK_KHR_SURFACE_EXTENSION_NAME) == 0) hasSurface = true;
        if (strcmp(e.extensionName, VK_KHR_ANDROID_SURFACE_EXTENSION_NAME) == 0)
            hasAndroidSurface = true;
    }
    if (!hasSurface || !hasAndroidSurface) {
        VERR("loader Vulkan sem extensions de superfície (surface=%d android=%d)",
             hasSurface ? 1 : 0, hasAndroidSurface ? 1 : 0);
        return false;
    }

    const char* enabled[] = {VK_KHR_SURFACE_EXTENSION_NAME,
                             VK_KHR_ANDROID_SURFACE_EXTENSION_NAME};
    VkApplicationInfo app{VK_STRUCTURE_TYPE_APPLICATION_INFO};
    app.pApplicationName = "FH2-Recomp";
    app.apiVersion = VK_API_VERSION_1_1;
    VkInstanceCreateInfo ci{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
    ci.pApplicationInfo = &app;
    ci.enabledExtensionCount = 2;
    ci.ppEnabledExtensionNames = enabled;

    const VkResult r = vkCreateInstance(&ci, nullptr, &instance_);
    if (r != VK_SUCCESS) {
        VERR("vkCreateInstance: %s — caindo para GLES", vkResultStr(r));
        instance_ = VK_NULL_HANDLE;
        return false;
    }
    instanceReady_ = true;
    VLOG("instance criada (Vulkan 1.1, %s + %s)", VK_KHR_SURFACE_EXTENSION_NAME,
         VK_KHR_ANDROID_SURFACE_EXTENSION_NAME);
    return true;
}

bool VulkanBackend::ensureDevice() {
    if (deviceReady_) return true;
    if (!instanceReady_) return false;

    // Physical device: primeiro com fila gráfica (o device Android tem 1 GPU;
    // preferência real por relojoeamento alto ficaria aqui sem fake).
    uint32_t gpuCount = 0;
    if (vkEnumeratePhysicalDevices(instance_, &gpuCount, nullptr) != VK_SUCCESS ||
        gpuCount == 0) {
        VERR("nenhum physical device Vulkan");
        return false;
    }
    std::vector<VkPhysicalDevice> gpus(gpuCount);
    vkEnumeratePhysicalDevices(instance_, &gpuCount, gpus.data());
    for (VkPhysicalDevice gpu : gpus) {
        uint32_t famCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(gpu, &famCount, nullptr);
        std::vector<VkQueueFamilyProperties> fams(famCount);
        vkGetPhysicalDeviceQueueFamilyProperties(gpu, &famCount, fams.data());
        for (uint32_t i = 0; i < famCount; ++i) {
            if (fams[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                pdev_ = gpu;
                queueFamily_ = i;
                break;
            }
        }
        if (pdev_ != VK_NULL_HANDLE) break;
    }
    if (pdev_ == VK_NULL_HANDLE) {
        VERR("nenhum physical device com fila gráfica");
        return false;
    }

    // Present na mesma família? (Android: quase sempre sim; se não, tenta
    // outra família com present — fallback REAL, não silencioso.)
    VkBool32 presentOk = VK_FALSE;
    vkGetPhysicalDeviceSurfaceSupportKHR(pdev_, queueFamily_, surface_, &presentOk);
    if (!presentOk) {
        uint32_t famCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(pdev_, &famCount, nullptr);
        for (uint32_t i = 0; i < famCount; ++i) {
            VkBool32 p = VK_FALSE;
            vkGetPhysicalDeviceSurfaceSupportKHR(pdev_, i, surface_, &p);
            if (p) {
                queueFamily_ = i;
                presentOk = VK_TRUE;
                VLOG("present em família separada (%u)", i);
                break;
            }
        }
        if (!presentOk) {
            VERR("surface sem fila de present — caindo para GLES");
            return false;
        }
    }

    VkPhysicalDeviceProperties props{};
    vkGetPhysicalDeviceProperties(pdev_, &props);

    const float prio = 1.0f;
    VkDeviceQueueCreateInfo qci{VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
    qci.queueFamilyIndex = queueFamily_;
    qci.queueCount = 1;
    qci.pQueuePriorities = &prio;

    const char* devExts[] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
    VkDeviceCreateInfo dci{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
    dci.queueCreateInfoCount = 1;
    dci.pQueueCreateInfos = &qci;
    dci.enabledExtensionCount = 1;
    dci.ppEnabledExtensionNames = devExts;

    const VkResult r = vkCreateDevice(pdev_, &dci, nullptr, &device_);
    if (r != VK_SUCCESS) {
        VERR("vkCreateDevice: %s", vkResultStr(r));
        return false;
    }
    vkGetDeviceQueue(device_, queueFamily_, 0, &queue_);
    deviceReady_ = true;
    VLOG("device ativo: %s (fila %u, %s)", props.deviceName, queueFamily_,
         props.vendorID == 0x5143 ? "Adreno" : "GPU Vulkan");
    return true;
}

bool VulkanBackend::provisionalInit() {
    return ensureInstance();
}

// ---------------------------------------------------------- superfície

bool VulkanBackend::ensureSurface(ANativeWindow* window) {
    if (surface_ != VK_NULL_HANDLE && window_ == window) return true;
    if (surface_ != VK_NULL_HANDLE) {
        vkDestroySurfaceKHR(instance_, surface_, nullptr);
        surface_ = VK_NULL_HANDLE;
    }
    VkAndroidSurfaceCreateInfoKHR sci{VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR};
    sci.window = window;
    const VkResult r = vkCreateAndroidSurfaceKHR(instance_, &sci, nullptr, &surface_);
    if (r != VK_SUCCESS) {
        VERR("vkCreateAndroidSurfaceKHR: %s", vkResultStr(r));
        return false;
    }
    window_ = window;
    VLOG("VkSurfaceKHR criada p/ ANativeWindow %p", (void*)window);
    return true;
}

bool VulkanBackend::createSwapchainObjects() {
    VkSurfaceCapabilitiesKHR caps{};
    if (vkGetPhysicalDeviceSurfaceCapabilitiesKHR(pdev_, surface_, &caps) != VK_SUCCESS) {
        VERR("vkGetPhysicalDeviceSurfaceCapabilitiesKHR falhou");
        return false;
    }

    // Formato: prefere RGBA8/BGRA8 UNORM (equivalente ao front buffer X8R8G8B8)
    uint32_t fmtCount = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(pdev_, surface_, &fmtCount, nullptr);
    std::vector<VkSurfaceFormatKHR> fmts(fmtCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(pdev_, surface_, &fmtCount, fmts.data());
    swapFormat_ = fmts[0].format;
    for (const auto& f : fmts) {
        if ((f.format == VK_FORMAT_R8G8B8A8_UNORM || f.format == VK_FORMAT_B8G8R8A8_UNORM) &&
            f.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            swapFormat_ = f.format;
            break;
        }
    }

    // Extent: no Android currentExtent é válido e é o tamanho REAL da janela.
    // (resolutionScale_ aplica-se aos render targets do Xenos — issue #17;
    //  sem blit real, escalar o swapchain seria fake.)
    extent_ = caps.currentExtent;
    width_ = int(extent_.width);
    height_ = int(extent_.height);

    uint32_t imageCount = caps.minImageCount + 1;
    if (caps.maxImageCount > 0 && imageCount > caps.maxImageCount) {
        imageCount = caps.maxImageCount;
    }

    // Present mode: FIFO é obrigatório e = vblank do display (60/30 fps alvo)
    VkSwapchainCreateInfoKHR sci{VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR};
    sci.surface = surface_;
    sci.minImageCount = imageCount;
    sci.imageFormat = swapFormat_;
    sci.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    sci.imageExtent = extent_;
    sci.imageArrayLayers = 1;
    sci.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    sci.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    sci.queueFamilyIndexCount = 1;
    sci.pQueueFamilyIndices = &queueFamily_;
    sci.preTransform = caps.currentTransform;
    sci.compositeAlpha = (caps.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR)
                             ? VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR
                             : VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR;
    sci.presentMode = VK_PRESENT_MODE_FIFO_KHR;
    sci.clipped = VK_TRUE;

    const VkResult r = vkCreateSwapchainKHR(device_, &sci, nullptr, &swapchain_);
    if (r != VK_SUCCESS) {
        VERR("vkCreateSwapchainKHR: %s", vkResultStr(r));
        return false;
    }

    uint32_t n = 0;
    vkGetSwapchainImagesKHR(device_, swapchain_, &n, nullptr);
    images_.resize(n);
    vkGetSwapchainImagesKHR(device_, swapchain_, &n, images_.data());

    // Render pass: um attachment de cor, CLEAR→STORE, UNDEFINED→PRESENT_SRC
    VkAttachmentDescription att{};
    att.format = swapFormat_;
    att.samples = VK_SAMPLE_COUNT_1_BIT;
    att.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    att.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    att.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    att.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    att.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    att.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    VkAttachmentReference ref{0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
    VkSubpassDescription sub{};
    sub.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    sub.colorAttachmentCount = 1;
    sub.pColorAttachments = &ref;
    VkSubpassDependency dep{};
    dep.srcSubpass = VK_SUBPASS_EXTERNAL;
    dep.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dep.srcAccessMask = 0;
    dep.dstSubpass = 0;
    dep.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dep.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    VkRenderPassCreateInfo rpci{VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
    rpci.attachmentCount = 1;
    rpci.pAttachments = &att;
    rpci.subpassCount = 1;
    rpci.pSubpasses = &sub;
    rpci.dependencyCount = 1;
    rpci.pDependencies = &dep;
    if (vkCreateRenderPass(device_, &rpci, nullptr, &renderPass_) != VK_SUCCESS) {
        VERR("vkCreateRenderPass falhou");
        destroySwapchainObjects();
        return false;
    }

    // Image views + framebuffers + command buffers + sync por imagem
    views_.resize(n);
    framebuffers_.resize(n);
    for (uint32_t i = 0; i < n; ++i) {
        VkImageViewCreateInfo vci{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
        vci.image = images_[i];
        vci.viewType = VK_IMAGE_VIEW_TYPE_2D;
        vci.format = swapFormat_;
        vci.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        vci.subresourceRange.levelCount = 1;
        vci.subresourceRange.layerCount = 1;
        if (vkCreateImageView(device_, &vci, nullptr, &views_[i]) != VK_SUCCESS) {
            VERR("vkCreateImageView[%u] falhou", i);
            destroySwapchainObjects();
            return false;
        }
        VkFramebufferCreateInfo fci{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
        fci.renderPass = renderPass_;
        fci.attachmentCount = 1;
        fci.pAttachments = &views_[i];
        fci.width = extent_.width;
        fci.height = extent_.height;
        fci.layers = 1;
        if (vkCreateFramebuffer(device_, &fci, nullptr, &framebuffers_[i]) != VK_SUCCESS) {
            VERR("vkCreateFramebuffer[%u] falhou", i);
            destroySwapchainObjects();
            return false;
        }
    }

    VkCommandPoolCreateInfo pci{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
    pci.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    pci.queueFamilyIndex = queueFamily_;
    if (vkCreateCommandPool(device_, &pci, nullptr, &cmdPool_) != VK_SUCCESS) {
        VERR("vkCreateCommandPool falhou");
        destroySwapchainObjects();
        return false;
    }
    cmds_.resize(n);
    VkCommandBufferAllocateInfo cai{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    cai.commandPool = cmdPool_;
    cai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cai.commandBufferCount = n;
    if (vkAllocateCommandBuffers(device_, &cai, cmds_.data()) != VK_SUCCESS) {
        VERR("vkAllocateCommandBuffers falhou");
        destroySwapchainObjects();
        return false;
    }

    imageAvailable_.assign(kMaxInFlight, VK_NULL_HANDLE);
    renderFinished_.assign(kMaxInFlight, VK_NULL_HANDLE);
    inFlight_.assign(kMaxInFlight, VK_NULL_HANDLE);
    VkSemaphoreCreateInfo semi{VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
    VkFenceCreateInfo fence{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
    fence.flags = VK_FENCE_CREATE_SIGNALED_BIT; // 1º acquire não espera à toa
    for (uint32_t i = 0; i < kMaxInFlight; ++i) {
        if (vkCreateSemaphore(device_, &semi, nullptr, &imageAvailable_[i]) != VK_SUCCESS ||
            vkCreateSemaphore(device_, &semi, nullptr, &renderFinished_[i]) != VK_SUCCESS ||
            vkCreateFence(device_, &fence, nullptr, &inFlight_[i]) != VK_SUCCESS) {
            VERR("sync[%u] falhou", i);
            destroySwapchainObjects();
            return false;
        }
    }

    surfaceValid_ = true;
    VLOG("swapchain ativo: %ux%u fmt=0x%X %u imagens (FIFO/vblank)", extent_.width,
         extent_.height, swapFormat_, n);
    return true;
}

void VulkanBackend::destroySwapchainObjects() {
    surfaceValid_ = false;
    if (device_ == VK_NULL_HANDLE) {
        return; // nada a destruir (evita refs a handles mortos)
    }
    for (VkFence f : inFlight_)
        if (f != VK_NULL_HANDLE) vkDestroyFence(device_, f, nullptr);
    for (VkSemaphore s : imageAvailable_)
        if (s != VK_NULL_HANDLE) vkDestroySemaphore(device_, s, nullptr);
    for (VkSemaphore s : renderFinished_)
        if (s != VK_NULL_HANDLE) vkDestroySemaphore(device_, s, nullptr);
    inFlight_.clear();
    imageAvailable_.clear();
    renderFinished_.clear();
    if (cmdPool_ != VK_NULL_HANDLE) {
        vkDestroyCommandPool(device_, cmdPool_, nullptr);
        cmdPool_ = VK_NULL_HANDLE;
    }
    cmds_.clear();
    for (VkFramebuffer fb : framebuffers_)
        if (fb != VK_NULL_HANDLE) vkDestroyFramebuffer(device_, fb, nullptr);
    framebuffers_.clear();
    for (VkImageView v : views_)
        if (v != VK_NULL_HANDLE) vkDestroyImageView(device_, v, nullptr);
    views_.clear();
    if (renderPass_ != VK_NULL_HANDLE) {
        vkDestroyRenderPass(device_, renderPass_, nullptr);
        renderPass_ = VK_NULL_HANDLE;
    }
    if (swapchain_ != VK_NULL_HANDLE) {
        vkDestroySwapchainKHR(device_, swapchain_, nullptr);
        swapchain_ = VK_NULL_HANDLE;
    }
    images_.clear();
}

bool VulkanBackend::recreateSwapchain() {
    destroySwapchainObjects();
    if (!createSwapchainObjects()) {
        VERR("recreate do swapchain falhou — frame próximo apresentará erro");
        return false;
    }
    return true;
}

void VulkanBackend::destroyDeviceAndInstance() {
    destroySwapchainObjects();
    destroyFrontBufferResources();
    if (surface_ != VK_NULL_HANDLE) {
        vkDestroySurfaceKHR(instance_, surface_, nullptr);
        surface_ = VK_NULL_HANDLE;
    }
    window_ = nullptr;
    if (device_ != VK_NULL_HANDLE) {
        vkDestroyDevice(device_, nullptr);
        device_ = VK_NULL_HANDLE;
    }
    deviceReady_ = false;
    if (instance_ != VK_NULL_HANDLE) {
        vkDestroyInstance(instance_, nullptr);
        instance_ = VK_NULL_HANDLE;
    }
    instanceReady_ = false;
    pdev_ = VK_NULL_HANDLE;
}

bool VulkanBackend::onSurfaceAvailable(ANativeWindow* window, int width, int height) {
    std::lock_guard<std::mutex> lock(gpuMutex_);
    width_ = width;
    height_ = height;
    if (!ensureInstance()) return false;
    if (!deviceReady_ || surface_ == VK_NULL_HANDLE || window_ != window) {
        // device precisa da surface p/ validar present — (re)cria na ordem
        destroyDeviceAndInstance();
        if (!ensureInstance() || !ensureSurface(window)) return false;
        if (!ensureDevice()) return false;
    }
    if (!createSwapchainObjects()) return false;
    VLOG("surface: %dx%d (escala %d%% aplica-se aos targets do Xenos — issue #17)",
         width_, height_, resolutionScale_.load());
    return true;
}

void VulkanBackend::onSurfaceLost() {
    std::lock_guard<std::mutex> lock(gpuMutex_);
    if (instance_ != VK_NULL_HANDLE || device_ != VK_NULL_HANDLE) {
        destroyDeviceAndInstance();
        VLOG("surface perdida: recursos Vulkan liberados");
    }
}

// ------------------------------------------------------------- present



bool VulkanBackend::present() {
    return presentInternal(nullptr, 0, 0, 0, 0);
}

bool VulkanBackend::presentFrontBuffer(const void* data, uint32_t rowBytes,
                                       uint32_t w, uint32_t h,
                                       uint32_t xenosFormat) {
    // Só k_8_8_8_8 (6) neste marco — os outros formatos ficam para o próximo
    // passo da issue #17 (o CP não chama este método para eles).
    if (xenosFormat != 6 || data == nullptr) return false;
    return presentInternal(data, rowBytes, w, h, xenosFormat);
}

bool VulkanBackend::ensureFrontBufferResources(uint32_t rowBytes, uint32_t w,
                                               uint32_t h) {
    const VkDeviceSize needed = VkDeviceSize(rowBytes) * h;
    if (stagingBuffer_ != VK_NULL_HANDLE && stagingSize_ >= needed &&
        fbImage_ != VK_NULL_HANDLE && fbWidth_ == w && fbHeight_ == h) {
        return true;
    }

    vkDeviceWaitIdle(device_);
    destroyFrontBufferResources();

    // Staging (host visible, linear) — upload do front buffer guest.
    VkBufferCreateInfo sbi{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    sbi.size = needed;
    sbi.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    sbi.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    if (vkCreateBuffer(device_, &sbi, nullptr, &stagingBuffer_) != VK_SUCCESS) {
        VERR("front buffer: vkCreateBuffer staging falhou");
        return false;
    }
    VkMemoryRequirements req;
    vkGetBufferMemoryRequirements(device_, stagingBuffer_, &req);
    uint32_t memType = 0xFFFFFFFFu;
    VkPhysicalDeviceMemoryProperties memProps;
    vkGetPhysicalDeviceMemoryProperties(pdev_, &memProps);
    for (uint32_t i = 0; i < memProps.memoryTypeCount; ++i) {
        if ((req.memoryTypeBits & (1u << i)) &&
            (memProps.memoryTypes[i].propertyFlags &
             (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
              VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)) != 0) {
            memType = i;
            break;
        }
    }
    if (memType == 0xFFFFFFFFu) {
        VERR("front buffer: sem memória host visible");
        return false;
    }
    VkMemoryAllocateInfo mai{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
    mai.allocationSize = req.size;
    mai.memoryTypeIndex = memType;
    if (vkAllocateMemory(device_, &mai, nullptr, &stagingMemory_) != VK_SUCCESS) {
        VERR("front buffer: vkAllocateMemory staging falhou");
        return false;
    }
    vkBindBufferMemory(device_, stagingBuffer_, stagingMemory_, 0);
    if (vkMapMemory(device_, stagingMemory_, 0, needed, 0,
                    (void**)&stagingMapped_) != VK_SUCCESS) {
        VERR("front buffer: vkMapMemory falhou");
        return false;
    }
    stagingSize_ = needed;

    // Imagem linear do front buffer (formato do título: k_8_8_8_8 → R8G8B8A8).
    VkImageCreateInfo ici{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
    ici.imageType = VK_IMAGE_TYPE_2D;
    ici.format = VK_FORMAT_R8G8B8A8_UNORM;
    ici.extent = {w, h, 1};
    ici.mipLevels = 1;
    ici.arrayLayers = 1;
    ici.samples = VK_SAMPLE_COUNT_1_BIT;
    ici.tiling = VK_IMAGE_TILING_LINEAR;
    ici.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    ici.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    ici.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    if (vkCreateImage(device_, &ici, nullptr, &fbImage_) != VK_SUCCESS) {
        VERR("front buffer: vkCreateImage falhou");
        return false;
    }
    vkGetImageMemoryRequirements(device_, fbImage_, &req);
    memType = 0xFFFFFFFFu;
    for (uint32_t i = 0; i < memProps.memoryTypeCount; ++i) {
        if ((req.memoryTypeBits & (1u << i)) &&
            (memProps.memoryTypes[i].propertyFlags &
             VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) != 0) {
            memType = i;
            break;
        }
    }
    if (memType == 0xFFFFFFFFu) memType = mai.memoryTypeIndex;
    mai.allocationSize = req.size;
    mai.memoryTypeIndex = memType;
    if (vkAllocateMemory(device_, &mai, nullptr, &fbMemory_) != VK_SUCCESS) {
        VERR("front buffer: vkAllocateMemory imagem falhou");
        return false;
    }
    vkBindImageMemory(device_, fbImage_, fbMemory_, 0);
    fbWidth_ = w;
    fbHeight_ = h;
    fbFormat_ = 6; // k_8_8_8_8 — único formato neste caminho
    VLOG("front buffer: recursos prontos (%ux%u, %llu bytes staging)",
         w, h, (unsigned long long)needed);
    return true;
}

void VulkanBackend::destroyFrontBufferResources() {
    if (stagingBuffer_ != VK_NULL_HANDLE) {
        vkDestroyBuffer(device_, stagingBuffer_, nullptr);
        stagingBuffer_ = VK_NULL_HANDLE;
    }
    if (stagingMapped_ != nullptr && stagingMemory_ != VK_NULL_HANDLE) {
        vkUnmapMemory(device_, stagingMemory_);
        stagingMapped_ = nullptr;
    }
    if (stagingMemory_ != VK_NULL_HANDLE) {
        vkFreeMemory(device_, stagingMemory_, nullptr);
        stagingMemory_ = VK_NULL_HANDLE;
    }
    if (fbImage_ != VK_NULL_HANDLE) {
        vkDestroyImage(device_, fbImage_, nullptr);
        fbImage_ = VK_NULL_HANDLE;
    }
    if (fbMemory_ != VK_NULL_HANDLE) {
        vkFreeMemory(device_, fbMemory_, nullptr);
        fbMemory_ = VK_NULL_HANDLE;
    }
    stagingSize_ = 0;
    fbWidth_ = fbHeight_ = fbFormat_ = 0;
}

bool VulkanBackend::presentInternal(const void* data, uint32_t rowBytes,
                                    uint32_t w, uint32_t h,
                                    uint32_t xenosFormat) {
    std::lock_guard<std::mutex> lock(gpuMutex_);
    if (!surfaceValid_ || device_ == VK_NULL_HANDLE) return false;

    if (data != nullptr) {
        if (!ensureFrontBufferResources(rowBytes, w, h)) return false;
        // Snapshot REAL do front buffer guest (bytes BE = R,G,B,A em k_8_8_8_8).
        memcpy(stagingMapped_, data, size_t(rowBytes) * h);
    }

    // Slot do frame em voo: fence garantido sinalizado antes do reset (o
    // acquire usa um semáforo cuja submissão anterior já terminou — o
    // vkWaitForFences do slot passou).
    const uint32_t f = uint32_t(frameCounter_ % kMaxInFlight);
    vkWaitForFences(device_, 1, &inFlight_[f], VK_TRUE, UINT64_MAX);

    uint32_t idx = 0;
    VkResult r = vkAcquireNextImageKHR(device_, swapchain_, UINT64_MAX,
                                       imageAvailable_[f], VK_NULL_HANDLE, &idx);
    if (r == VK_ERROR_OUT_OF_DATE_KHR || r == VK_ERROR_SURFACE_LOST_KHR) {
        if (!recreateSwapchain()) return false;
        r = vkAcquireNextImageKHR(device_, swapchain_, UINT64_MAX,
                                  imageAvailable_[f], VK_NULL_HANDLE, &idx);
        if (r != VK_SUCCESS && r != VK_SUBOPTIMAL_KHR) return false;
    } else if (r != VK_SUCCESS && r != VK_SUBOPTIMAL_KHR) {
        VERR("acquire: %s", vkResultStr(r));
        return false;
    }

    vkResetFences(device_, 1, &inFlight_[f]);

    VkCommandBuffer cb = cmds_[idx];
    vkResetCommandBuffer(cb, 0);
    VkCommandBufferBeginInfo bi{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    bi.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    if (vkBeginCommandBuffer(cb, &bi) != VK_SUCCESS) {
        recreateFenceSignaled(f);
        return false;
    }

    if (data == nullptr) {
        // Sem frame do título: render pass com clear PRETO. O backend NÃO
        // inventa conteúdo — tela colorida aqui fingiria que o jogo roda.
        VkClearValue clear{};
        clear.color = {{0.0f, 0.0f, 0.0f, 1.0f}};
        VkRenderPassBeginInfo rp{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
        rp.renderPass = renderPass_;
        rp.framebuffer = framebuffers_[idx];
        rp.renderArea.extent = extent_;
        rp.clearValueCount = 1;
        rp.pClearValues = &clear;
        vkCmdBeginRenderPass(cb, &rp, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdEndRenderPass(cb);
    } else {
        // Frame REAL do título: copy staging → imagem → blit p/ o swapchain.
        VkImageMemoryBarrier toDst{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
        toDst.srcAccessMask = 0;
        toDst.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        toDst.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        toDst.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        toDst.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        toDst.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        toDst.image = fbImage_;
        toDst.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
        vkCmdPipelineBarrier(cb, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                             VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr,
                             0, nullptr, 1, &toDst);

        VkBufferImageCopy region{};
        region.bufferOffset = 0;
        region.bufferRowLength = rowBytes / 4; // texels (pitch do guest)
        region.bufferImageHeight = 0;
        region.imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
        region.imageExtent = {w, h, 1};
        vkCmdCopyBufferToImage(cb, stagingBuffer_, fbImage_,
                               VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1,
                               &region);

        VkImageMemoryBarrier toSrc = toDst;
        toSrc.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        toSrc.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        toSrc.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        toSrc.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        vkCmdPipelineBarrier(cb, VK_PIPELINE_STAGE_TRANSFER_BIT,
                             VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr,
                             0, nullptr, 1, &toSrc);

        VkImageMemoryBarrier swToDst{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
        swToDst.srcAccessMask = 0;
        swToDst.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        swToDst.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        swToDst.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        swToDst.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        swToDst.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        swToDst.image = images_[idx];
        swToDst.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
        vkCmdPipelineBarrier(cb, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                             VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr,
                             0, nullptr, 1, &swToDst);

        VkImageBlit blit{};
        blit.srcSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
        blit.srcOffsets[1] = {int32_t(fbWidth_), int32_t(fbHeight_), 1};
        blit.dstSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
        blit.dstOffsets[1] = {int32_t(extent_.width), int32_t(extent_.height),
                              1};
        vkCmdBlitImage(cb, fbImage_, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                       images_[idx], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1,
                       &blit, VK_FILTER_LINEAR);

        VkImageMemoryBarrier swToPresent = swToDst;
        swToPresent.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        swToPresent.dstAccessMask = 0;
        swToPresent.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        swToPresent.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        vkCmdPipelineBarrier(cb, VK_PIPELINE_STAGE_TRANSFER_BIT,
                             VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0,
                             nullptr, 0, nullptr, 1, &swToPresent);
    }

    if (vkEndCommandBuffer(cb) != VK_SUCCESS) {
        recreateFenceSignaled(f);
        return false;
    }

    // O acquire precisa valer no estágio que toca a imagem do swapchain:
    // render pass escreve em COLOR_ATTACHMENT_OUTPUT; o blit escreve em
    // TRANSFER — cada caminho com o wait correto (sem race com o compositor).
    VkPipelineStageFlags waitStage =
        (data == nullptr) ? VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
                          : VK_PIPELINE_STAGE_TRANSFER_BIT;
    VkSubmitInfo si{VK_STRUCTURE_TYPE_SUBMIT_INFO};
    si.waitSemaphoreCount = 1;
    si.pWaitSemaphores = &imageAvailable_[f];
    si.pWaitDstStageMask = &waitStage;
    si.commandBufferCount = 1;
    si.pCommandBuffers = &cb;
    si.signalSemaphoreCount = 1;
    si.pSignalSemaphores = &renderFinished_[f];
    if (vkQueueSubmit(queue_, 1, &si, inFlight_[f]) != VK_SUCCESS) {
        VERR("vkQueueSubmit falhou");
        recreateFenceSignaled(f);
        return false;
    }

    VkPresentInfoKHR pi{VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
    pi.waitSemaphoreCount = 1;
    pi.pWaitSemaphores = &renderFinished_[f];
    pi.swapchainCount = 1;
    pi.pSwapchains = &swapchain_;
    pi.pImageIndices = &idx;
    r = vkQueuePresentKHR(queue_, &pi);
    ++frameCounter_;

    if (r == VK_ERROR_OUT_OF_DATE_KHR || r == VK_SUBOPTIMAL_KHR) {
        recreateSwapchain();
    } else if (r != VK_SUCCESS) {
        VERR("present: %s", vkResultStr(r));
        return false;
    }

    // Telemetria REAL de present (a cada 300 frames; primeira amostra
    // descartada — o intervalo inclui o boot e não representa fps)
    if (frameCounter_ - lastFpsLog_ >= 300) {
        auto now = std::chrono::steady_clock::now();
        if (lastPresentLog_ != std::chrono::steady_clock::time_point{}) {
            double dt = std::chrono::duration<double>(now - lastPresentLog_).count();
            if (dt > 0.5) {
                VLOG("present: %llu frames no swapchain (%.1f fps medidos)",
                     (unsigned long long)frameCounter_, 300.0 / dt);
            }
        }
        lastPresentLog_ = now;
        lastFpsLog_ = frameCounter_;
    }
    return true;
}

void VulkanBackend::recreateFenceSignaled(uint32_t slot) {
    if (slot >= kMaxInFlight) return;
    if (inFlight_[slot] != VK_NULL_HANDLE) {
        vkDestroyFence(device_, inFlight_[slot], nullptr);
    }
    VkFenceCreateInfo fence{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
    fence.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    if (vkCreateFence(device_, &fence, nullptr, &inFlight_[slot]) != VK_SUCCESS) {
        inFlight_[slot] = VK_NULL_HANDLE;
    }
}

} // namespace fh2::vulkan
