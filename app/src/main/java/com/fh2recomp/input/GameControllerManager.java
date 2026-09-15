package com.fh2recomp.input;

import android.view.InputDevice;
import android.view.KeyEvent;
import android.view.MotionEvent;

import com.fh2recomp.nativebridge.NativeBridge;

/**
 * GameControllerManager — suporte a gamepad físico (Bluetooth/USB).
 *
 * Mapeamento idêntico ao controle original do Xbox 360:
 *  - Triggers ANALÓGICOS: AXIS_BRAKE (esq) e AXIS_GAS (dir) — acelerador/freio
 *  - Sticks: AXIS_X/Y (direção), AXIS_Z/RZ (câmera); HAT_X/Y → D-pad
 *  - LB/RB = câmbio -, +; X = freio de mão; Y = câmera; Start = pause
 *
 * Envia o keycode Android BRUTO ao runtime — a tradução para o formato do
 * guest (máscara XInput-like) vive no native (input_state.cpp), que conhece
 * os códigos AKEYCODE_BUTTON_x e AKEYCODE_DPAD_x e mantém um único mapa canônico.
 *
 * Portas: cada deviceId é atribuído a uma porta estável 0..3 na ordem de
 * chegada (o guest suporta 4 controllers).
 */
public final class GameControllerManager {

    private static final android.util.SparseIntArray deviceToPort = new android.util.SparseIntArray();
    private static int nextPort = 0;

    /** Porta estável 0..3 para o deviceId (atribuição em ordem de chegada). */
    private static int portFor(int deviceId) {
        int port = deviceToPort.get(deviceId, -1);
        if (port == -1) {
            if (nextPort >= 4) return -1; // limite do guest
            port = nextPort++;
            deviceToPort.put(deviceId, port);
        }
        return port;
    }

    public static boolean handleKey(int deviceId, KeyEvent e) {
        int keyCode = e.getKeyCode();
        if (!isGamepadKey(keyCode)) return false;
        int port = portFor(deviceId);
        if (port < 0) return true; // consome, sem porta disponível
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

    public static boolean handleMotion(int deviceId, MotionEvent e) {
        if ((e.getSource() & InputDevice.SOURCE_CLASS_JOYSTICK) == 0) return false;
        int port = portFor(deviceId);
        if (port < 0) return true;
        NativeBridge.onGamepadAxis(port, MotionEvent.AXIS_X, e.getAxisValue(MotionEvent.AXIS_X));        // stick L
        NativeBridge.onGamepadAxis(port, MotionEvent.AXIS_Y, e.getAxisValue(MotionEvent.AXIS_Y));
        NativeBridge.onGamepadAxis(port, MotionEvent.AXIS_Z, e.getAxisValue(MotionEvent.AXIS_Z));        // stick R
        NativeBridge.onGamepadAxis(port, MotionEvent.AXIS_RZ, e.getAxisValue(MotionEvent.AXIS_RZ));
        NativeBridge.onGamepadAxis(port, MotionEvent.AXIS_GAS, e.getAxisValue(MotionEvent.AXIS_GAS));    // trigger direito (analógico)
        NativeBridge.onGamepadAxis(port, MotionEvent.AXIS_BRAKE, e.getAxisValue(MotionEvent.AXIS_BRAKE));// trigger esquerdo (analógico)
        NativeBridge.onGamepadAxis(port, MotionEvent.AXIS_HAT_X, e.getAxisValue(MotionEvent.AXIS_HAT_X));// D-pad
        NativeBridge.onGamepadAxis(port, MotionEvent.AXIS_HAT_Y, e.getAxisValue(MotionEvent.AXIS_HAT_Y));
        return true;
    }
}
