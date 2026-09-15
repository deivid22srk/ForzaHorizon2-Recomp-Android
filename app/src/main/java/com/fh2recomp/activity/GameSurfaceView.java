package com.fh2recomp.activity;

import android.content.Context;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;

import com.fh2recomp.nativebridge.NativeBridge;

/**
 * GameSurfaceView — SurfaceView dedicada ao renderizador nativo.
 * Trata criação/destruição da superfície (perda de contexto ao minimizar).
 */
public class GameSurfaceView extends SurfaceView implements SurfaceHolder.Callback {

    private NativeBridge bridge;
    private String assetsUri;
    private int resolutionScale;
    private boolean fps60;
    private boolean useVulkan;
    private boolean booted;

    public GameSurfaceView(Context context) {
        super(context);
        getHolder().addCallback(this);
    }

    void setHost(GameActivity activity, NativeBridge bridge, String assetsUri,
                 int resolutionScale, boolean fps60, boolean useVulkan) {
        this.bridge = bridge;
        this.assetsUri = assetsUri;
        this.resolutionScale = resolutionScale;
        this.fps60 = fps60;
        this.useVulkan = useVulkan;
    }

    @Override
    public void surfaceCreated(SurfaceHolder holder) {
        if (bridge == null) return;
        Surface s = holder.getSurface();
        if (!booted) {
            booted = true;
            // Boot assíncrono: evita travar a UI thread durante o init do runtime
            new Thread(() -> {
                if (bridge.nativeBoot(assetsUri, resolutionScale, fps60, useVulkan)) {
                    bridge.nativeSetSurface(s, holder.getSurfaceFrame().width(),
                            holder.getSurfaceFrame().height());
                }
            }, "fh2-boot").start();
        } else {
            bridge.nativeSetSurface(s, holder.getSurfaceFrame().width(),
                    holder.getSurfaceFrame().height());
        }
    }

    @Override
    public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
        if (bridge != null && booted) {
            bridge.nativeSetSurface(holder.getSurface(), width, height);
        }
    }

    @Override
    public void surfaceDestroyed(SurfaceHolder holder) {
        // Runtime solta a superfície; contexto gráfico pode ser perdido
        if (bridge != null) bridge.nativePause();
    }
}
