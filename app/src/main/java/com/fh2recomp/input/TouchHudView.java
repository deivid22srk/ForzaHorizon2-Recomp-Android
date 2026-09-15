package com.fh2recomp.input;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.util.AttributeSet;
import android.view.HapticFeedbackConstants;
import android.view.MotionEvent;
import android.view.View;

/**
 * TouchHudView — HUD de controles touch pensado para DIREÇÃO (não para combate):
 *  - Analógico virtual à esquerda: direção do carro
 *  - Botões à direita: acelerador, freio/ré, freio de mão
 *  - Botões inferiores: câmbio (cima/baixo), câmera, buzina, pause
 *
 * MULTI-TOUCH CORRETO: cada controle rastreia seu próprio pointer ID — é
 * possível dirigir (stick) com o polegar esquerdo enquanto o direito acelera
 * e freia simultaneamente; soltar o acelerador NÃO interfere na direção.
 * Métricas em dp (independentes de densidade); opacidade aplicada a TODAS as
 * cores desenhadas; feedback háptico nos botões.
 */
public class TouchHudView extends View {

    /** IDs de botão — mapeiam para o estado do controller virtual (port 0). */
    public interface ButtonId {
        int ACCEL = 0;        // gás (trigger direito analógico ~1.0)
        int BRAKE = 1;        // freio / ré
        int HANDBRAKE = 2;    // freio de mão
        int GEAR_UP = 3;      // câmbio +
        int GEAR_DOWN = 4;    // câmbio -
        int CAMERA = 5;       // alterna câmera
        int HORN = 6;         // buzina
        int PAUSE = 7;        // pause
    }

    public interface ButtonListener { void onButton(int buttonId, boolean pressed); }
    public interface StickListener { void onMove(float x, float y); }

    private static final int NO_POINTER = -1;
    private static final int NO_BUTTON = -1;

    private final Paint paint = new Paint(Paint.ANTI_ALIAS_FLAG);
    private ButtonListener buttonListener;
    private StickListener stickListener;
    private float opacity = 0.7f;
    private float density = 1f;

    // Estado do stick — rastreado por pointer ID (multi-touch correto)
    private int stickPointerId = NO_POINTER;
    private float stickX, stickY;          // -1..1 (origem no centro do stick)
    private float stickCenterX, stickCenterY, stickRadius;

    // Botões: cada pointer pressionado guarda o botão que acionou
    private final android.util.SparseIntArray pointerToButton = new android.util.SparseIntArray();
    private final android.util.SparseBooleanArray buttonPressed = new android.util.SparseBooleanArray();

    // Métricas (em px, calculadas de dp em onLayout)
    private float accelX, accelY, brakeX, brakeY, handX;
    private float gearUpX, gearDownX, camX, hornX, pauseX;
    private float buttonR;

    private static final class Hit { final int id; final float x; final float y; final float r;
        Hit(int id, float x, float y, float r) { this.id = id; this.x = x; this.y = y; this.r = r; } }

    public TouchHudView(Context c, AttributeSet a) {
        super(c, a);
        density = getResources().getDisplayMetrics().density;
    }

    /** Opacidade [0.1..1] aplicada ao canal alpha de TODAS as cores do desenho. */
    public void setOpacity(float o) {
        opacity = Math.max(0.1f, Math.min(1f, o));
        invalidate();
    }
    public void setButtonListener(ButtonListener l) { buttonListener = l; }
    public void setStickListener(StickListener l) { stickListener = l; }

    private float dp(float v) { return v * density; }

    /** Multiplica o canal alpha de uma cor pela opacidade configurada. */
    private int opaque(int color) {
        int a = Math.round(((color >>> 24) / 255f) * opacity * 255f);
        return (a << 24) | (color & 0x00FFFFFF);
    }

    @Override
    protected void onLayout(boolean changed, int l, int t, int r, int b) {
        super.onLayout(changed, l, t, r, b);
        if (!changed) return;
        float w = r - l, h = b - t;
        stickRadius = Math.min(h * 0.16f, dp(120));
        buttonR = Math.min(h * 0.075f, dp(56));

        stickCenterX = stickRadius + dp(60);
        stickCenterY = h - stickRadius - dp(60);

        // Coluna direita: acelerador acima do freio (polegar direito)
        accelX = w - buttonR * 3.1f;  accelY = h - buttonR * 4.6f;
        brakeX = w - buttonR * 1.6f;  brakeY = h - buttonR * 2.4f;
        handX  = w - buttonR * 5.0f;  // freio de mão ao lado do acelerador

        // Linha inferior (após o stick): câmbio, câmera, buzina; pause à direita
        gearDownX = stickCenterX + buttonR * 2.6f;  gearUpX = gearDownX + buttonR * 2.2f;
        camX = gearUpX + buttonR * 2.2f;  hornX = camX + buttonR * 2.2f;  pauseX = w - buttonR * 1.4f;
    }

    private float bottomY() { return getHeight() - buttonR * 1.2f; }

    private Hit[] hits() {
        return new Hit[] {
            new Hit(ButtonId.ACCEL, accelX, accelY, buttonR),
            new Hit(ButtonId.BRAKE, brakeX, brakeY, buttonR),
            new Hit(ButtonId.HANDBRAKE, handX, accelY, buttonR * 0.9f),
            new Hit(ButtonId.GEAR_UP, gearUpX, bottomY(), buttonR * 0.9f),
            new Hit(ButtonId.GEAR_DOWN, gearDownX, bottomY(), buttonR * 0.9f),
            new Hit(ButtonId.CAMERA, camX, bottomY(), buttonR * 0.9f),
            new Hit(ButtonId.HORN, hornX, bottomY(), buttonR * 0.9f),
            new Hit(ButtonId.PAUSE, pauseX, bottomY(), buttonR * 0.9f),
        };
    }

    @Override
    protected void onDraw(Canvas canvas) {
        // Stick direcional
        paint.setStyle(Paint.Style.STROKE);
        paint.setStrokeWidth(dp(6));
        paint.setColor(opaque(Color.LTGRAY));
        canvas.drawCircle(stickCenterX, stickCenterY, stickRadius, paint);
        paint.setStyle(Paint.Style.FILL);
        paint.setColor(opaque(Color.WHITE));
        canvas.drawCircle(stickCenterX + stickX * (stickRadius * 0.6f),
                stickCenterY + stickY * (stickRadius * 0.6f), stickRadius * 0.38f, paint);

        // Botões
        for (Hit hit : hits()) {
            drawButton(canvas, hit.x, hit.y, hit.r, labelFor(hit.id), buttonPressed.get(hit.id, false));
        }
    }

    private String labelFor(int id) {
        switch (id) {
            case ButtonId.ACCEL: return "GAS";
            case ButtonId.BRAKE: return "FREIO";
            case ButtonId.HANDBRAKE: return "MÃO";
            case ButtonId.GEAR_UP: return "C+";
            case ButtonId.GEAR_DOWN: return "C-";
            case ButtonId.CAMERA: return "CAM";
            case ButtonId.HORN: return "H";
            case ButtonId.PAUSE: return "||";
            default: return "?";
        }
    }

    private void drawButton(Canvas c, float x, float y, float r, String label, boolean pressed) {
        paint.setStyle(Paint.Style.FILL);
        paint.setColor(opaque(pressed ? Color.rgb(61, 220, 132) : Color.argb(255, 30, 40, 52)));
        c.drawCircle(x, y, r, paint);
        paint.setStyle(Paint.Style.STROKE);
        paint.setStrokeWidth(dp(3));
        paint.setColor(opaque(Color.LTGRAY));
        c.drawCircle(x, y, r, paint);
        paint.setStyle(Paint.Style.FILL);
        paint.setColor(opaque(Color.WHITE));
        paint.setTextSize(r * 0.55f);
        paint.setTextAlign(Paint.Align.CENTER);
        c.drawText(label, x, y + r * 0.2f, paint);
    }

    @Override
    public boolean onTouchEvent(MotionEvent ev) {
        switch (ev.getActionMasked()) {
            case MotionEvent.ACTION_DOWN:
            case MotionEvent.ACTION_POINTER_DOWN: {
                int idx = ev.getActionIndex();
                int pid = ev.getPointerId(idx);
                float x = ev.getX(idx), y = ev.getY(idx);
                if (stickPointerId == NO_POINTER
                        && Math.hypot(x - stickCenterX, y - stickCenterY) <= stickRadius * 1.35f) {
                    stickPointerId = pid;
                } else {
                    int btn = hitButton(x, y);
                    if (btn != NO_BUTTON && buttonPressed.get(btn, false) == false) {
                        pointerToButton.put(pid, btn);
                        buttonPressed.put(btn, true);
                        if (buttonListener != null) buttonListener.onButton(btn, true);
                        performHapticFeedback(HapticFeedbackConstants.VIRTUAL_KEY);
                        invalidate();
                    }
                }
                return true;
            }
            case MotionEvent.ACTION_MOVE: {
                // atualiza TODOS os pointers ativos (stick acompanha seu dedo,
                // mesmo que ele não seja o pointer 0)
                if (stickPointerId != NO_POINTER) {
                    int idx = ev.findPointerIndex(stickPointerId);
                    if (idx >= 0) {
                        float dx = ev.getX(idx) - stickCenterX;
                        float dy = ev.getY(idx) - stickCenterY;
                        float len = (float) Math.hypot(dx, dy);
                        float clamped = Math.min(len, stickRadius);
                        float nx = len > 0 ? dx / len : 0, ny = len > 0 ? dy / len : 0;
                        stickX = nx * (clamped / stickRadius);
                        stickY = ny * (clamped / stickRadius);
                        if (stickListener != null) stickListener.onMove(stickX, stickY);
                    }
                }
                return true;
            }
            case MotionEvent.ACTION_POINTER_UP: {
                int idx = ev.getActionIndex();
                releasePointer(ev.getPointerId(idx));
                return true;
            }
            case MotionEvent.ACTION_UP:
            case MotionEvent.ACTION_CANCEL: {
                releaseAll();
                return true;
            }
        }
        return super.onTouchEvent(ev);
    }

    private int hitButton(float x, float y) {
        for (Hit hit : hits()) {
            if (Math.hypot(x - hit.x, y - hit.y) <= hit.r * 1.25f) return hit.id;
        }
        return NO_BUTTON;
    }

    private void releasePointer(int pid) {
        if (pid == stickPointerId) {
            stickPointerId = NO_POINTER;
            stickX = stickY = 0;
            if (stickListener != null) stickListener.onMove(0, 0);
            invalidate();
            return;
        }
        int btn = pointerToButton.get(pid, NO_BUTTON);
        if (btn != NO_BUTTON) {
            pointerToButton.delete(pid);
            buttonPressed.put(btn, false);
            if (buttonListener != null) buttonListener.onButton(btn, false);
            invalidate();
        }
    }

    private void releaseAll() {
        if (stickPointerId != NO_POINTER) {
            stickPointerId = NO_POINTER;
            stickX = stickY = 0;
            if (stickListener != null) stickListener.onMove(0, 0);
        }
        for (int i = 0; i < pointerToButton.size(); i++) {
            int btn = pointerToButton.valueAt(i);
            if (buttonPressed.get(btn, false)) {
                buttonPressed.put(btn, false);
                if (buttonListener != null) buttonListener.onButton(btn, false);
            }
        }
        pointerToButton.clear();
        invalidate();
    }
}
