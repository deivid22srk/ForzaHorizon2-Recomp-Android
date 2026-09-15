// input_state.h — estado consolidado de entrada (guest vê um "X360 controller")
//
// Camadas combinadas:
//  1. HUD touch virtual (stick direção + botões)  — port 0
//  2. Gamepad físico via GameController (BT/USB)  — ports 0..3
//  3. Triggers analógicos de gamepad (AXIS_GAS/BRAKE) — acelerador/freio
#pragma once

#include <array>
#include <atomic>
#include <cstdint>

namespace fh2::input {

// Códigos de botão no formato do guest (XInput-like: 16 bits de máscara)
enum GuestButton : uint16_t {
    kDpadUp = 0x0001, kDpadDown = 0x0002, kDpadLeft = 0x0004, kDpadRight = 0x0008,
    kStart = 0x0010, kBack = 0x0020,
    kL3 = 0x0040, kR3 = 0x0080,
    kLB = 0x0100, kRB = 0x0200,
    kA = 0x1000, kB = 0x2000, kX = 0x4000, kY = 0x8000,
};

struct ControllerState {
    uint16_t buttons = 0;
    float leftX = 0.f, leftY = 0.f;     // direção
    float rightX = 0.f, rightY = 0.f;   // câmera
    float leftTrigger = 0.f;            // freio/ré (analógico)
    float rightTrigger = 0.f;           // acelerador (analógico)
};

class InputState {
public:
    static constexpr int kMaxPorts = 4;

    // --- HUD virtual (port 0) ---
    void setVirtualButton(int hudButtonId, bool pressed);
    void setVirtualStick(float x, float y);

    // --- Gamepad físico (Android KeyEvent/MotionEvent codes) ---
    void setGamepadButton(int port, int androidButtonCode, bool pressed);
    void setGamepadAxis(int port, int androidAxisCode, float value);

    /** Snapshot atômico para a thread do guest. */
    ControllerState snapshot(int port) const;

private:
    static ControllerState& slot(int port) {
        static std::array<ControllerState, kMaxPorts> s{};
        return s[port];
    }
    static std::array<std::atomic<int32_t>, kMaxPorts>& buttonMaskAtomic();
};

} // namespace fh2::input
