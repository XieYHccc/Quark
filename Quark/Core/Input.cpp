#include "Quark/Core/Input.h"
#include <GLFW/glfw3.h>
#include "Quark/Core/Window.h"

namespace quark {

bool Input::IsKeyPressed(KeyCode key, bool repeat) const
{
    if (repeat) {
        return keyMouseStatus_[key] == KeyAction::KEY_PRESSED
            || keyMouseStatus_[key] == KeyAction::KEY_KEEP_PRESSED;
    }
    else
        return keyMouseStatus_[key] == KeyAction::KEY_PRESSED;
}


bool Input::IsKeyReleased(KeyCode key) const
{
    return keyMouseStatus_[key] == KeyAction::KEY_RELEASED;
}

bool Input::IsKeyKeepPressed(KeyCode key) const
{
    return keyMouseStatus_[key] == KeyAction::KEY_KEEP_PRESSED;
}

bool Input::IsMousePressed(MouseCode button, bool repeat) const
{
    return IsKeyPressed(button, repeat);
}

bool Input::IsMouseReleased(MouseCode button) const
{
    return keyMouseStatus_[button] == KeyAction::KEY_RELEASED;
}


MousePosition Input::GetMousePosition() const
{
    return mousePosition_;
}

}
