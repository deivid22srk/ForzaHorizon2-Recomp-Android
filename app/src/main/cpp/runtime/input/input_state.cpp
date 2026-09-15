// input_state.cpp — implementação do estado de entrada
//
// Os botões do guest ficam em máscara atômica (leitura snapshot lock-free).
// HUD touch e gamepad físico contribuem no mesmo estado (port 0).
#include "input_state.h"

#include <android/input.h>
#include <algorithm>

namespace fh2::input {

std::array<std::atomic<int32_t>, kMaxPorts>& buttonMaskAtomic() {
    static std::array<std::atomic<int32_t>, kMaxPorts> masks{};
    return masks;
}

// --- HUD virtual: TouchHudView.ButtonId → botões guest ---

void InputState::setVirtualButton(int hudButtonId, bool pressed) {
    int32_t bit = 0;
    float* trigger = nullptr;
    switch (hudButtonId) {
        case 0: trigger = &slot(0).rightTrigger; break;         // ACCEL (analógico ~1.0)
        case 1: trigger = &slot(0).leftTrigger; break;          // BRAKE
        case 2: bit = kX; break;                                // HANDBRAKE
        case 3: bit = kRB; break;                               // GEAR_UP
        case 4: bit = kLB; break;                               // GEAR_DOWN
        case 5: bit = kY; break;                                // CAMERA
        case 6: bit = kR3; break;                               // HORN
        case 7: bit = kStart; break;                            // PAUSE
        default: return;
    }
    if (trigger) {
        *trigger = pressed ? 1.f : 0.f;
    } else if (bit) {
        auto& m = buttonMaskAtomic()[0];
        if (pressed) m.fetch_or(bit);
        else m.fetch_and(~bit);
    }
}

void InputState::setVirtualStick(float x, float y) {
    slot(0).leftX = std::clamp(x, -1.f, 1.f);
    slot(0).leftY = std::clamp(y, -1.f, 1.f);
}

// --- Gamepad físico: Android codes → botões guest (mapa Xbox 360) ---

void InputState::setGamepadButton(int port, int androidButtonCode, bool pressed) {
    if (port < 0 || port >= kMaxPorts) return;
    uint16_t bit = 0;
    switch (androidButtonCode) {
        case AKEYCODE_BUTTON_A: bit = kA; break;
        case AKEYCODE_BUTTON_B: bit = kB; break;
        case AKEYCODE_BUTTON_X: bit = kX; break;      // freio de mão
        case AKEYCODE_BUTTON_Y: bit = kY; break;      // câmera
        case AKEYCODE_BUTTON_L1: bit = kLB; break;    // câmbio -
        case AKEYCODE_BUTTON_R1: bit = kRB; break;    // câmbio +
        case AKEYCODE_BUTTON_START: bit = kStart; break;
        case AKEYCODE_BUTTON_SELECT: bit = kBack; break;
        case AKEYCODE_BUTTON_THUMBL: bit = kL3; break;
        case AKEYCODE_BUTTON_THUMBR: bit = kR3; break;
        case AKEYCODE_DPAD_UP: bit = kDpadUp; break;
        case AKEYCODE_DPAD_DOWN: bit = kDpadDown; break;
        case AKEYCODE_DPAD_LEFT: bit = kDpadLeft; break;
        case AKEYCODE_DPAD_RIGHT: bit = kDpadRight; break;
        default: return;
    }
    auto& m = buttonMaskAtomic()[port];
    if (pressed) m.fetch_or(bit);
    else m.fetch_and(~bit);
}

void InputState::setGamepadAxis(int port, int androidAxisCode, float value) {
    if (port < 0 || port >= kMaxPorts) return;
    auto& c = slot(port);
    switch (androidAxisCode) {
        case AMOTION_EVENT_AXIS_X: c.leftX = std::clamp(value, -1.f, 1.f); break;
        case AMOTION_EVENT_AXIS_Y: c.leftY = std::clamp(value, -1.f, 1.f); break;
        case AMOTION_EVENT_AXIS_Z: c.rightX = std::clamp(value, -1.f, 1.f); break;
        case AMOTION_EVENT_AXIS_RZ: c.rightY = std::clamp(value, -1.f, 1.f); break;
        case AMOTION_EVENT_AXIS_GAS: c.rightTrigger = std::clamp(value, 0.f, 1.f); break;
        case AMOTION_EVENT_AXIS_BRAKE: c.leftTrigger = std::clamp(value, 0.f, 1.f); break;
        default: break; // hat axis tratado como botões via KeyEvent
    }
}

ControllerState InputState::snapshot(int port) const {
    if (port < 0 || port >= kMaxPorts) return {};
    ControllerState s;
    s.buttons = uint16_t(buttonMaskAtomic()[port].load());
    const auto& src = slot(port);
    s.leftX = src.leftX; s.leftY = src.leftY;
    s.rightX = src.rightX; s.rightY = src.rightY;
    s.leftTrigger = src.leftTrigger; s.rightTrigger = src.rightTrigger;
    return s;
}

} // namespace fh2::input
