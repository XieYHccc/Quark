#include "Core/Input.h"
#include <GLFW/glfw3.h>
#include "Core/Window.h"

bool Input::IsKeyPressed(Keycode key, bool repeat) const
{
    if (repeat) {
        return keyMouseStatus_[key] == KeyAction::KEY_PRESSED
            || keyMouseStatus_[key] == KeyAction::KEY_KEEP_PRESSED;
    }
    else
        return keyMouseStatus_[key] == KeyAction::KEY_PRESSED;
}


bool Input::IsKeyReleased(Keycode key) const
{
    return keyMouseStatus_[key] == KeyAction::KEY_RELEASED;
}

bool Input::IsKeyKeepPressed(Keycode key) const
{
    return keyMouseStatus_[key] == KeyAction::KEY_KEEP_PRESSED;
}

bool Input::IsMousePressed(MouseCode button) const
{
    return keyMouseStatus_[button] == KeyAction::KEY_PRESSED;
}

bool Input::IsMouseReleased(MouseCode button) const
{
    return keyMouseStatus_[button] == KeyAction::KEY_RELEASED;
}



MousePosition Input::GetMousePosition() const
{
    return mousePosition_;
}



