package com.fh2recomp.input;

import android.view.InputDevice;
import android.view.KeyEvent;
import android.view.MotionEvent;

import com.fh2recomp.nativebridge.NativeBridge;

/**
 * GameControllerManager — suporte a gamepad físico (Bluetooth/USB).
 *
 * Mapeamento idêntico ao controle original do Xbox 360:
 *  - A (acelerar não é A no FH2; A é usado para menus/aceitar)
 *  - Triggers ANALÓGICOS: AXIS_BRAKE (esq) e AXIS_GAS (dir) — acelerador/freio
 *  - Sticks: AXIS_X/Y (direção), AXIS_Z/RZ (câmera)
 *  - LB/RB = câmbio -, +; X = freio de mão; Y = câmera; Start = pause
 */
public final class GameControllerManager {

    /**
     * Processa KeyEvents do gamepad (chamado de GameActivity.dispatchKeyEvent).
     *
     * Envia o keycode Android BRUTO ao runtime — a tradução para o formato do
     * guest (máscara XInput-like) vive no native (input_state.cpp), que conhece
     * AKEYCODE_BUTTON_*/AKEYCODE_DPAD_* e mantém um único mapa canônico.
     */
    public static boolean handleKey(int port, KeyEvent e) {
        int keyCode = e.getKeyCode();
        if (!isGamepadKey(keyCode)) return false;
        if (e.getAction() != KeyEvent.ACTION_DOWN && e.getAction() != KeyEvent.ACTION_UP) return true;
        boolean pressed = e.getAction() == KeyEvent.ACTION_DOWN;
        boolean repeat = pressed && e.getRepeatCount() > 0;
        if (!repeat) NativeBridge.onGamepadEvent(port, keyCode, pressed);
        return true;
    }

    private static boolean isGamepadKey(int keyCode) {
        switch (keyCode) {
            case KeyEvent.KEYCODE_BUTTON_A:
            case KeyEvent.KEYCODE_BUTTON_B:
            case KeyEvent.KEYCODE_BUTTON_X:
            case KeyEvent.KEYCODE_BUTTON_Y:
            case KeyEvent.KEYCODE_BUTTON_L1:
            case KeyEvent.KEYCODE_BUTTON_R1:
            case KeyEvent.KEYCODE_BUTTON_L2:
            case KeyEvent.KEYCODE_BUTTON_R2:
            case KeyEvent.KEYCODE_BUTTON_START:
            case KeyEvent.KEYCODE_BUTTON_SELECT:
            case KeyEvent.KEYCODE_BUTTON_THUMBL:
            case KeyEvent.KEYCODE_BUTTON_THUMBR:
            case KeyEvent.KEYCODE_DPAD_UP:
            case KeyEvent.KEYCODE_DPAD_DOWN:
            case KeyEvent.KEYCODE_DPAD_LEFT:
            case KeyEvent.KEYCODE_DPAD_RIGHT:
                return true;
            default:
                return false;
        }
    }

    /** Processa MotionEvents do gamepad (chamado de GameActivity.dispatchGenericMotionEvent). */
    public static boolean handleMotion(int port, MotionEvent e) {
        if (!e.isFromSource(InputDevice.SOURCE_CLASS_JOYSTICK)) return false;
        NativeBridge.onGamepadAxis(port, MotionEvent.AXIS_X, e.getAxisValue(MotionEvent.AXIS_X));        // stick L
        NativeBridge.onGamepadAxis(port, MotionEvent.AXIS_Y, e.getAxisValue(MotionEvent.AXIS_Y));
        NativeBridge.onGamepadAxis(port, MotionEvent.AXIS_Z, e.getAxisValue(MotionEvent.AXIS_Z));        // stick R
        NativeBridge.onGamepadAxis(port, MotionEvent.AXIS_RZ, e.getAxisValue(MotionEvent.AXIS_RZ));
        NativeBridge.onGamepadAxis(port, MotionEvent.AXIS_GAS, e.getAxisValue(MotionEvent.AXIS_GAS));    // trigger direito (analógico)
        NativeBridge.onGamepadAxis(port, MotionEvent.AXIS_BRAKE, e.getAxisValue(MotionEvent.AXIS_BRAKE));// trigger esquerdo (analógico)
        return true;
    }
}
