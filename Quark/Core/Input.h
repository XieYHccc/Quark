#pragma once
#include "Quark/Core/KeyMouseCodes.h"
#include "Quark/Core/Util/Singleton.h"

namespace quark {

// TODO: Design a efficient input system
struct MousePosition 
{
    float x_pos;
    float y_pos;
};

enum class KeyAction {
    KEY_RELEASED = 0,
    KEY_PRESSED = 1,
    KEY_KEEP_PRESSED = 2,
};

class Input :public util::MakeSingletonPtr<Input> {
public:
    Input() = default;
    virtual ~Input() = default;
    
    virtual void Init() = 0; // Init mouse position
    virtual void OnUpdate() = 0;  // pool input event
    virtual void Finalize() = 0;

    bool IsKeyPressed(KeyCode key, bool repeat) const;
    bool IsKeyReleased(KeyCode key) const;
    bool IsKeyKeepPressed(KeyCode key) const;

    bool IsMousePressed(MouseCode button, bool repeat) const;
    bool IsMouseReleased(MouseCode button) const;
    bool IsMouseKeepPressed(MouseCode button) const;

    MousePosition GetMousePosition() const;
protected:
    KeyAction keyMouseStatus_[512] = {};
    MousePosition mousePosition_ = {};
};

// Implemmented in platform specific code
template <>
template <>
Input* util::MakeSingletonPtr<Input>::CreateSingleton();

}