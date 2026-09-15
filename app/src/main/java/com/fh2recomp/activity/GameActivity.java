package com.fh2recomp.activity;

import android.net.Uri;
import android.os.Bundle;

import androidx.appcompat.app.AppCompatActivity;

import com.fh2recomp.R;
import com.fh2recomp.input.TouchHudView;
import com.fh2recomp.nativebridge.NativeBridge;

/**
 * GameActivity — hospeda a SurfaceView de renderização e o HUD touch.
 *
 * Ciclo de vida Android: pausa/resume e perda de contexto gráfico são
 * tratados via callbacks nativos (obrigatório em mobile, inexistente em ports desktop).
 */
public class GameActivity extends AppCompatActivity {

    private NativeBridge nativeBridge;
    private TouchHudView hud;
    private boolean pausedState;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_game);

        Uri assetsUri = getIntent().getData();
        int resolutionScale = getIntent().getIntExtra("resolution_scale", 75);
        boolean fps60 = getIntent().getBooleanExtra("fps_60", true);
        boolean useVulkan = getIntent().getBooleanExtra("use_vulkan", false);
        int hudOpacity = getIntent().getIntExtra("hud_opacity", 70);

        nativeBridge = new NativeBridge();
        hud = findViewById(R.id.touch_hud);
        hud.setOpacity(hudOpacity / 100f);
        // Eventos do HUD alimentam o estado do "controller virtual" (port 0)
        hud.setButtonListener((button, pressed) -> NativeBridge.onVirtualButton(button, pressed));
        hud.setStickListener((x, y) -> NativeBridge.onVirtualStick(x, y));
        // Surface criada pelo GameSurfaceView dispara o boot do runtime nativo
        GameSurfaceView surface = findViewById(R.id.game_surface);
        surface.setHost(this, nativeBridge,
                assetsUri != null ? assetsUri.toString() : null,
                resolutionScale, fps60, useVulkan);
    }

    @Override
    protected void onPause() {
        super.onPause();
        if (nativeBridge != null) { nativeBridge.nativePause(); pausedState = true; }
    }

    @Override
    protected void onResume() {
        super.onResume();
        if (nativeBridge != null) { nativeBridge.nativeResume(); pausedState = false; }
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        if (nativeBridge != null) nativeBridge.nativeStop();
    }

    @Override
    public void onBackPressed() {
        // Botão back = pause; segunda vez sai (evita encerrar o jogo sem querer)
        if (nativeBridge != null && !pausedState) {
            nativeBridge.nativePause();
            pausedState = true;
            android.widget.Toast.makeText(this,
                    "Pausado — pressione voltar novamente para sair",
                    android.widget.Toast.LENGTH_LONG).show();
        } else {
            super.onBackPressed();
        }
    }

    // --- Gamepad físico (Bluetooth/USB): despacha para GameControllerManager ---

    @Override
    public boolean dispatchKeyEvent(android.view.KeyEvent event) {
        if (GameControllerManager.handleKey(0, event)) return true;
        return super.dispatchKeyEvent(event);
    }

    @Override
    public boolean dispatchGenericMotionEvent(android.view.MotionEvent event) {
        if (GameControllerManager.handleMotion(0, event)) return true;
        return super.dispatchGenericMotionEvent(event);
    }
}
