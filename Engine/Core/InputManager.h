#pragma once
#include "Core/KeyMouseCodes.h"
#include "Util/Singleton.h"

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

class InputManager :public MakeSingletonPtr<InputManager> {
public:
    InputManager() = default;
    virtual ~InputManager() = default;
    
    virtual void Init() = 0; // Init mouse position
    virtual void Update() = 0;  // pool input event
    virtual void Finalize() = 0;

    bool IsKeyPressed(Keycode key, bool repeat) const;
    bool IsKeyReleased(Keycode key) const;
    bool IsKeyKeepPressed(Keycode key) const;

    bool IsMousePressed(MouseCode button) const;
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
InputManager* MakeSingletonPtr<InputManager>::CreateSingleton();
