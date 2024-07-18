#include "Core/InputManager.h"
#include <GLFW/glfw3.h>
#include "Core/Window.h"

bool InputManager::IsKeyPressed(Keycode key, bool repeat) const
{
    if (repeat) {
        return keyMouseStatus_[key] == KeyAction::KEY_PRESSED
            || keyMouseStatus_[key] == KeyAction::KEY_KEEP_PRESSED;
    }
    else
        return keyMouseStatus_[key] == KeyAction::KEY_PRESSED;
}


bool InputManager::IsKeyReleased(Keycode key) const
{
    return keyMouseStatus_[key] == KeyAction::KEY_RELEASED;
}

bool InputManager::IsKeyKeepPressed(Keycode key) const
{
    return keyMouseStatus_[key] == KeyAction::KEY_KEEP_PRESSED;
}

bool InputManager::IsMousePressed(MouseCode button) const
{
    return keyMouseStatus_[button] == KeyAction::KEY_PRESSED;
}

bool InputManager::IsMouseReleased(MouseCode button) const
{
    return keyMouseStatus_[button] == KeyAction::KEY_RELEASED;
}



MousePosition InputManager::GetMousePosition() const
{
    return mousePosition_;
}



