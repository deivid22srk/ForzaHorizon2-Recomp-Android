package com.fh2recomp.activity;

import android.net.Uri;
import android.os.Bundle;
import android.view.WindowManager;

import androidx.appcompat.app.AppCompatActivity;
import androidx.core.view.WindowCompat;
import androidx.core.view.WindowInsetsCompat;
import androidx.core.view.WindowInsetsControllerCompat;

import com.fh2recomp.R;
import com.fh2recomp.input.GameControllerManager;
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
        WindowCompat.setDecorFitsSystemWindows(getWindow(), false);
        setContentView(R.layout.activity_game);
        // Tela cheia durante o jogo (barras ocultas, swipe para exibir)
        WindowInsetsControllerCompat insets =
                new WindowInsetsControllerCompat(getWindow(), getWindow().getDecorView());
        insets.hide(WindowInsetsCompat.Type.systemBars());
        insets.setSystemBarsBehavior(
                WindowInsetsControllerCompat.BEHAVIOR_SHOW_TRANSIENT_BARS_BY_SWIPE);
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);

        Uri assetsUri = getIntent().getData();
        boolean assetsIsIso = getIntent().getBooleanExtra("assets_is_iso", false);
        int resolutionScale = getIntent().getIntExtra("resolution_scale", 75);
        boolean fps60 = getIntent().getBooleanExtra("fps_60", true);
        boolean useVulkan = getIntent().getBooleanExtra("use_vulkan", true);
        int hudOpacity = getIntent().getIntExtra("hud_opacity", 70);

        // Contexto + URI SAF para a cópia lazy nativa, e filesDir para o FS
        NativeBridge.setAppContext(getApplicationContext(),
                assetsUri != null ? assetsUri.toString() : null);

        nativeBridge = new NativeBridge();
        nativeBridge.nativeSetFilesDir(getFilesDir().getAbsolutePath());

        hud = findViewById(R.id.touch_hud);
        hud.setOpacity(hudOpacity / 100f);
        // Eventos do HUD alimentam o estado do "controller virtual" (port 0)
        hud.setButtonListener((button, pressed) -> NativeBridge.onVirtualButton(button, pressed));
        hud.setStickListener((x, y) -> NativeBridge.onVirtualStick(x, y));
        // Surface criada pelo GameSurfaceView dispara o boot do runtime nativo
        GameSurfaceView surface = findViewById(R.id.game_surface);
        surface.setHost(this, nativeBridge,
                assetsUri != null ? assetsUri.toString() : null,
                assetsIsIso, resolutionScale, fps60, useVulkan);
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

    @Override
    public void onWindowFocusChanged(boolean hasFocus) {
        super.onWindowFocusChanged(hasFocus);
        if (hasFocus) {
            WindowInsetsControllerCompat insets =
                    new WindowInsetsControllerCompat(getWindow(), getWindow().getDecorView());
            insets.hide(WindowInsetsCompat.Type.systemBars());
            insets.setSystemBarsBehavior(
                    WindowInsetsControllerCompat.BEHAVIOR_SHOW_TRANSIENT_BARS_BY_SWIPE);
        }
    }

    // --- Gamepad físico (Bluetooth/USB): despacha para GameControllerManager ---

    @Override
    public boolean dispatchKeyEvent(android.view.KeyEvent event) {
        if (GameControllerManager.handleKey(event.getDeviceId(), event)) return true;
        return super.dispatchKeyEvent(event);
    }

    @Override
    public boolean dispatchGenericMotionEvent(android.view.MotionEvent event) {
        if (GameControllerManager.handleMotion(event.getDeviceId(), event)) return true;
        return super.dispatchGenericMotionEvent(event);
    }
}
