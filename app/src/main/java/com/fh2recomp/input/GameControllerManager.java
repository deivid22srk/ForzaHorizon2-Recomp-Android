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

    /** Processa KeyEvents do gamepad (chamado de GameActivity.dispatchKeyEvent). */
    public static boolean handleKey(int port, KeyEvent e) {
        boolean pressed = e.getAction() == KeyEvent.ACTION_DOWN;
        switch (e.getKeyCode()) {
            case KeyEvent.KEYCODE_BUTTON_A:      NativeBridge.onGamepadEvent(port, 0, pressed); return true; // A
            case KeyEvent.KEYCODE_BUTTON_B:      NativeBridge.onGamepadEvent(port, 1, pressed); return true; // B
            case KeyEvent.KEYCODE_BUTTON_X:      NativeBridge.onGamepadEvent(port, 2, pressed); return true; // X = freio de mão
            case KeyEvent.KEYCODE_BUTTON_Y:      NativeBridge.onGamepadEvent(port, 3, pressed); return true; // Y = câmera
            case KeyEvent.KEYCODE_BUTTON_L1:     NativeBridge.onGamepadEvent(port, 4, pressed); return true; // LB = câmbio -
            case KeyEvent.KEYCODE_BUTTON_R1:     NativeBridge.onGamepadEvent(port, 5, pressed); return true; // RB = câmbio +
            case KeyEvent.KEYCODE_BUTTON_START:  NativeBridge.onGamepadEvent(port, 6, pressed); return true; // Start
            case KeyEvent.KEYCODE_BUTTON_SELECT: NativeBridge.onGamepadEvent(port, 7, pressed); return true; // Back
            case KeyEvent.KEYCODE_DPAD_UP:       NativeBridge.onGamepadEvent(port, 8, pressed); return true;
            case KeyEvent.KEYCODE_DPAD_DOWN:     NativeBridge.onGamepadEvent(port, 9, pressed); return true;
            case KeyEvent.KEYCODE_DPAD_LEFT:     NativeBridge.onGamepadEvent(port, 10, pressed); return true;
            case KeyEvent.KEYCODE_DPAD_RIGHT:    NativeBridge.onGamepadEvent(port, 11, pressed); return true;
        }
        return false;
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
