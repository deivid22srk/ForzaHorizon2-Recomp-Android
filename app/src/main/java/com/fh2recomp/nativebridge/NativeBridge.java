package com.fh2recomp.nativebridge;

/**
 * Ponte JNI para o runtime nativo (libfh2recomp.so).
 *
 * O runtime nativo implementa: boot do código recompilado (PPC → C++),
 * backend gráfico (GLES 3.1 / Vulkan), áudio (AAudio) e entrada.
 */
public final class NativeBridge {

    static {
        System.loadLibrary("fh2recomp");
    }

    /**
     * Inicializa o runtime e inicia a thread principal do jogo.
     * @param assetsTreeUri URI SAF da pasta de assets fornecida pelo usuário
     * @return true se o boot do runtime iniciou com sucesso
     */
    public native boolean nativeBoot(String assetsTreeUri, int resolutionScalePct,
                                     boolean fps60, boolean useVulkan);

    /** Cria/realoca a superfície gráfica (chamado na UI thread via SurfaceHolder). */
    public native void nativeSetSurface(android.view.Surface surface, int width, int height);

    /** Ciclo de vida Android: pausado pelo sistema (perde foco, minimiza). */
    public native void nativePause();

    /** Ciclo de vida Android: retorna do pause (recria recursos perdidos se preciso). */
    public native void nativeResume();

    /** Encerramento definitivo. */
    public native void nativeStop();

    /** Botão virtual do HUD (códigos em TouchHudView.ButtonId). */
    public static native void onVirtualButton(int buttonId, boolean pressed);

    /** Analógico virtual do HUD (direção), valores normalizados [-1,1]. */
    public static native void onVirtualStick(float x, float y);

    /** Evento de gamepad físico (padrão Xbox 360). */
    public static native void onGamepadEvent(int controllerPort, int buttonCode, boolean pressed);

    /** Triggers analógicos e sticks de gamepad físico. */
    public static native void onGamepadAxis(int controllerPort, int axisCode, float value);
}
