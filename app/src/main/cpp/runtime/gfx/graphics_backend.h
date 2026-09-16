// graphics_backend.h — abstração do backend gráfico (GLES 3.1 / Vulkan)
//
// A camada de tradução D3D9/Xenos → APIs Android usa esta interface.
// Prioridade do projeto: Vulkan (Adreno/Turnip) com fallback GLES 3.1.
#pragma once

#include <cstdint>
#include <memory>

struct ANativeWindow;

namespace fh2::gfx {

enum class Backend {
    Gles3,
    Vulkan,
};

struct SurfaceInfo {
    uint32_t width = 0;
    uint32_t height = 0;
};

class GraphicsBackend {
public:
    virtual ~GraphicsBackend() = default;

    // Ciclo de vida Android: superfície pode ser recriada ao minimizar.
    virtual bool onSurfaceAvailable(ANativeWindow* window, int width, int height) = 0;
    virtual void onSurfaceLost() = 0;

    // Configuração (persistida em SharedPreferences via app Java)
    virtual void setResolutionScale(int pct) = 0;
    virtual void setFpsTarget(int fps) = 0;
    virtual int fpsTarget() const = 0;

    // Present REAL de um frame (flip): chamado por VdSwap (guest) e pelo
    // driver de frames do runtime enquanto o jogo não assumiu o present.
    // Bloqueia até o frame estar na tela (semântica de vblank do console);
    // retorna false se não há superfície válida (o chamador segue sem flip).
    virtual bool present() = 0;

    virtual const char* name() const = 0;
    virtual SurfaceInfo surfaceInfo() const = 0;
};

/** Backend ATIVO para o kernel (VdSwap): registrado no boot, limpo no stop. */
void setActiveBackend(GraphicsBackend* backend);
GraphicsBackend* activeBackend();

/** Cria o backend pedido; se Vulkan não estiver disponível no device, cai para GLES. */
std::unique_ptr<GraphicsBackend> createBackend(bool preferVulkan, int resolutionScalePct);

} // namespace fh2::gfx
