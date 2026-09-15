package com.fh2recomp.input;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.util.AttributeSet;
import android.view.MotionEvent;
import android.view.View;

/**
 * TouchHudView — HUD de controles touch pensado para DIREÇÃO (não para combate):
 *  - Analógico virtual à esquerda: direção do carro
 *  - Botões à direita: acelerador, freio/ré, freio de mão
 *  - Botões inferiores: câmbio (cima/baixo), câmera, horn
 *
 * Layout configurável (opacidade/escala via MainActivity).
 */
public class TouchHudView extends View {

    /** IDs de botão — mapeiam para o estado do controller virtual (port 0). */
    public interface ButtonId {
        int ACCEL = 0;        // A (X360) — mantém: acelerador
        int BRAKE = 1;        // B (X360) — freio / ré
        int HANDBRAKE = 2;    // X (X360) — freio de mão
        int GEAR_UP = 3;      // RB — câmbio +
        int GEAR_DOWN = 4;    // LB — câmbio -
        int CAMERA = 5;       // Y — alterna câmera
        int HORN = 6;         // RS click — buzina
        int PAUSE = 7;        // Start
    }

    public interface ButtonListener { void onButton(int buttonId, boolean pressed); }
    public interface StickListener { void onMove(float x, float y); }

    private final Paint paint = new Paint(Paint.ANTI_ALIAS_FLAG);
    private ButtonListener buttonListener;
    private StickListener stickListener;
    private float opacity = 0.7f;

    // Estado interno (para desenho)
    private float stickX, stickY;          // -1..1 (origem no centro do stick)
    private float stickCenterX, stickCenterY, stickRadius;
    private float accelX, accelY, brakeX, brakeY, handX;
    private float gearUpX, gearDownX, camX, hornX, pauseX;
    private float buttonR;
    private int pressedButton = -1;
    private boolean stickActive;

    private static final class Hit { final int id; final float x; final float y; final float r;
        Hit(int id, float x, float y, float r) { this.id = id; this.x = x; this.y = y; this.r = r; } }

    public TouchHudView(Context c, AttributeSet a) { super(c, a); }

    public void setOpacity(float o) { opacity = Math.max(0.1f, Math.min(1f, o)); invalidate(); }
    public void setButtonListener(ButtonListener l) { buttonListener = l; }
    public void setStickListener(StickListener l) { stickListener = l; }

    @Override
    protected void onLayout(boolean changed, int l, int t, int r, int b) {
        super.onLayout(changed, l, t, r, b);
        if (!changed) return;
        float w = r - l, h = b - t;
        stickRadius = Math.min(h * 0.16f, 120f);
        buttonR = Math.min(h * 0.075f, 56f);

        stickCenterX = stickRadius + 60f;
        stickCenterY = h - stickRadius - 60f;

        // Coluna direita: acelerador acima do freio (polegar direito)
        accelX = w - buttonR * 3.1f;  accelY = h - buttonR * 4.6f;
        brakeX = w - buttonR * 1.6f;  brakeY = h - buttonR * 2.4f;
        handX  = w - buttonR * 5.0f;  // freio de mão ao lado do acelerador

        // Linha inferior esquerda (após o stick): câmbio, câmera, horn, pause
        float y = h - buttonR * 1.2f;
        gearDownX = stickCenterX + buttonR * 2.6f;  gearUpX = gearDownX + buttonR * 2.2f;
        camX = gearUpX + buttonR * 2.2f;  hornX = camX + buttonR * 2.2f;  pauseX = w - buttonR * 1.4f;
    }

    @Override
    protected void onDraw(Canvas canvas) {
        paint.setAlpha((int) (opacity * 255));
        // Stick direcional
        paint.setStyle(Paint.Style.STROKE);
        paint.setStrokeWidth(6f);
        paint.setColor(Color.LTGRAY);
        canvas.drawCircle(stickCenterX, stickCenterY, stickRadius, paint);
        paint.setStyle(Paint.Style.FILL);
        paint.setColor(Color.WHITE);
        canvas.drawCircle(stickCenterX + stickX * (stickRadius * 0.6f),
                stickCenterY + stickY * (stickRadius * 0.6f), stickRadius * 0.38f, paint);

        // Botões
        drawButton(canvas, accelX, accelY, buttonR, "GAS", pressedButton == ButtonId.ACCEL);
        drawButton(canvas, brakeX, brakeY, buttonR, "FREIO", pressedButton == ButtonId.BRAKE);
        drawButton(canvas, handX, accelY, buttonR * 0.85f, "MÃO", pressedButton == ButtonId.HANDBRAKE);
        drawButton(canvas, gearUpX, y(), buttonR * 0.8f, "C+", pressedButton == ButtonId.GEAR_UP);
        drawButton(canvas, gearDownX, y(), buttonR * 0.8f, "C-", pressedButton == ButtonId.GEAR_DOWN);
        drawButton(canvas, camX, y(), buttonR * 0.8f, "CAM", pressedButton == ButtonId.CAMERA);
        drawButton(canvas, hornX, y(), buttonR * 0.8f, "H", pressedButton == ButtonId.HORN);
        drawButton(canvas, pauseX, y(), buttonR * 0.8f, "||", pressedButton == ButtonId.PAUSE);
    }

    private float y() { return getHeight() - buttonR * 1.2f; }

    private void drawButton(Canvas c, float x, float y, float r, String label, boolean pressed) {
        paint.setStyle(Paint.Style.FILL);
        paint.setColor(pressed ? Color.rgb(61, 220, 132) : Color.argb(255, 30, 40, 52));
        c.drawCircle(x, y, r, paint);
        paint.setStyle(Paint.Style.STROKE);
        paint.setStrokeWidth(3f);
        paint.setColor(Color.LTGRAY);
        c.drawCircle(x, y, r, paint);
        paint.setStyle(Paint.Style.FILL);
        paint.setColor(Color.WHITE);
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
                handleDown(ev.getX(idx), ev.getY(idx));
                return true;
            }
            case MotionEvent.ACTION_MOVE: {
                if (stickActive) {
                    float dx = ev.getX() - stickCenterX;
                    float dy = ev.getY() - stickCenterY;
                    float len = (float) Math.hypot(dx, dy);
                    float clamped = Math.min(len, stickRadius);
                    float nx = len > 0 ? dx / len : 0, ny = len > 0 ? dy / len : 0;
                    stickX = nx * (clamped / stickRadius);
                    stickY = ny * (clamped / stickRadius);
                    if (stickListener != null) stickListener.onMove(stickX, stickY);
                    invalidate();
                }
                return true;
            }
            case MotionEvent.ACTION_UP:
            case MotionEvent.ACTION_POINTER_UP: {
                int idx = ev.getActionIndex();
                if (stickActive) {
                    stickActive = false;
                    stickX = stickY = 0;
                    if (stickListener != null) stickListener.onMove(0, 0);
                } else if (pressedButton >= 0) {
                    releaseIfNear(ev.getX(idx), ev.getY(idx));
                }
                invalidate();
                return true;
            }
        }
        return super.onTouchEvent(ev);
    }

    private void handleDown(float x, float y) {
        if (Math.hypot(x - stickCenterX, y - stickCenterY) <= stickRadius * 1.35f) {
            stickActive = true;
            return;
        }
        int[] ids = { ButtonId.ACCEL, ButtonId.BRAKE, ButtonId.HANDBRAKE, ButtonId.GEAR_UP,
                      ButtonId.GEAR_DOWN, ButtonId.CAMERA, ButtonId.HORN, ButtonId.PAUSE };
        float[][] pos = {
            { accelX, accelY, buttonR }, { brakeX, brakeY, buttonR },
            { handX, accelY, buttonR * 0.9f },
            { gearUpX, y(), buttonR * 0.9f }, { gearDownX, y(), buttonR * 0.9f },
            { camX, y(), buttonR * 0.9f }, { hornX, y(), buttonR * 0.9f },
            { pauseX, y(), buttonR * 0.9f },
        };
        for (int i = 0; i < ids.length; i++) {
            if (Math.hypot(x - pos[i][0], y - pos[i][1]) <= pos[i][2] * 1.25f) {
                pressedButton = ids[i];
                if (buttonListener != null) buttonListener.onButton(ids[i], true);
                invalidate();
                return;
            }
        }
    }

    private void releaseIfNear(float x, float y) {
        if (buttonListener != null && pressedButton >= 0) {
            buttonListener.onButton(pressedButton, false);
        }
        pressedButton = -1;
    }
}
