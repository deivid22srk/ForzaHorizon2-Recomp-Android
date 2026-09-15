// input_state.cpp — implementação do estado de entrada
//
// Botões em máscara atômica; eixos em floats atômicos (relaxed) — escrita na
// thread de UI, leitura lock-free na thread do guest. Deadzone radial no stick.
#include "input_state.h"

#include <android/input.h>
#include <algorithm>
#include <cmath>

namespace fh2::input {

InputState::PortState& InputState::slot(int port) {
    static std::array<PortState, kMaxPorts> s{};
    return s[port];
}

float InputState::applyDeadzone(float v) {
    if (std::fabs(v) < kDeadzone) return 0.f;
    // reescala para usar toda a faixa após a deadzone
    float sign = v < 0.f ? -1.f : 1.f;
    return sign * (std::fabs(v) - kDeadzone) / (1.f - kDeadzone);
}

// --- HUD virtual: TouchHudView.ButtonId → botões guest ---

void InputState::setVirtualButton(int hudButtonId, bool pressed) {
    uint16_t bit = 0;
    std::atomic<float>* trigger = nullptr;
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
        trigger->store(pressed ? 1.f : 0.f, std::memory_order_relaxed);
    } else {
        auto& m = slot(0).buttons;
        if (pressed) m.fetch_or(bit, std::memory_order_relaxed);
        else m.fetch_and(uint32_t(~bit), std::memory_order_relaxed);
    }
}

void InputState::setVirtualStick(float x, float y) {
    slot(0).leftX.store(applyDeadzone(std::clamp(x, -1.f, 1.f)), std::memory_order_relaxed);
    slot(0).leftY.store(applyDeadzone(std::clamp(y, -1.f, 1.f)), std::memory_order_relaxed);
}

// --- Gamepad físico: Android codes → botões guest (mapa Xbox 360) ---

void InputState::setGamepadButton(int port, int androidButtonCode, bool pressed) {
    if (port < 0 || port >= kMaxPorts) return;
    auto& st = slot(port);
    // L2/R2 digitais: alguns pads enviam como botão além do eixo analógico
    std::atomic<float>* trigger = nullptr;
    uint16_t bit = 0;
    switch (androidButtonCode) {
        case AKEYCODE_BUTTON_A: bit = kA; break;
        case AKEYCODE_BUTTON_B: bit = kB; break;
        case AKEYCODE_BUTTON_X: bit = kX; break;      // freio de mão
        case AKEYCODE_BUTTON_Y: bit = kY; break;      // câmera
        case AKEYCODE_BUTTON_L1: bit = kLB; break;    // câmbio -
        case AKEYCODE_BUTTON_R1: bit = kRB; break;    // câmbio +
        case AKEYCODE_BUTTON_L2: trigger = &st.leftTrigger; break;
        case AKEYCODE_BUTTON_R2: trigger = &st.rightTrigger; break;
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
    if (trigger) {
        trigger->store(pressed ? 1.f : 0.f, std::memory_order_relaxed);
    } else {
        auto& m = st.buttons;
        if (pressed) m.fetch_or(bit, std::memory_order_relaxed);
        else m.fetch_and(uint32_t(~bit), std::memory_order_relaxed);
    }
}

void InputState::setGamepadAxis(int port, int androidAxisCode, float value) {
    if (port < 0 || port >= kMaxPorts) return;
    auto& st = slot(port);
    switch (androidAxisCode) {
        case AMOTION_EVENT_AXIS_X:  st.leftX.store(applyDeadzone(std::clamp(value, -1.f, 1.f)), std::memory_order_relaxed); break;
        case AMOTION_EVENT_AXIS_Y:  st.leftY.store(applyDeadzone(std::clamp(value, -1.f, 1.f)), std::memory_order_relaxed); break;
        case AMOTION_EVENT_AXIS_Z:  st.rightX.store(applyDeadzone(std::clamp(value, -1.f, 1.f)), std::memory_order_relaxed); break;
        case AMOTION_EVENT_AXIS_RZ: st.rightY.store(applyDeadzone(std::clamp(value, -1.f, 1.f)), std::memory_order_relaxed); break;
        case AMOTION_EVENT_AXIS_GAS:   st.rightTrigger.store(std::clamp(value, 0.f, 1.f), std::memory_order_relaxed); break;
        case AMOTION_EVENT_AXIS_BRAKE: st.leftTrigger.store(std::clamp(value, 0.f, 1.f), std::memory_order_relaxed); break;
        case AMOTION_EVENT_AXIS_HAT_X: {
            uint32_t set = value > 0.5f ? kDpadRight : (value < -0.5f ? kDpadLeft : 0);
            uint32_t clear = (kDpadLeft | kDpadRight) & ~set;
            st.buttons.fetch_or(set, std::memory_order_relaxed);
            st.buttons.fetch_and(~clear, std::memory_order_relaxed);
            break;
        }
        case AMOTION_EVENT_AXIS_HAT_Y: {
            uint32_t set = value > 0.5f ? kDpadDown : (value < -0.5f ? kDpadUp : 0);
            uint32_t clear = (kDpadUp | kDpadDown) & ~set;
            st.buttons.fetch_or(set, std::memory_order_relaxed);
            st.buttons.fetch_and(~clear, std::memory_order_relaxed);
            break;
        }
        default: break;
    }
}

ControllerSnapshot InputState::snapshot(int port) const {
    if (port < 0 || port >= kMaxPorts) return {};
    const auto& st = slot(port);
    ControllerSnapshot s;
    s.buttons = uint16_t(st.buttons.load(std::memory_order_relaxed));
    s.leftX = st.leftX.load(std::memory_order_relaxed);
    s.leftY = st.leftY.load(std::memory_order_relaxed);
    s.rightX = st.rightX.load(std::memory_order_relaxed);
    s.rightY = st.rightY.load(std::memory_order_relaxed);
    s.leftTrigger = st.leftTrigger.load(std::memory_order_relaxed);
    s.rightTrigger = st.rightTrigger.load(std::memory_order_relaxed);
    return s;
}

} // namespace fh2::input
