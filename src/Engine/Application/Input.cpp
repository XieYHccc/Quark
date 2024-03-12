#include "Application/Input.h"

#include <GLFW/glfw3.h>

#include "Application/Application.h"
#include "Application/Window/Window.h"

bool Input::first_mouse_ = true;
MousePosition Input::last_position_;

bool Input::IsKeyPressed(Keycode key)
{
    auto* window = Application::Instance().GetWindow().GetNativeWindow();
    auto state = glfwGetKey(window, key);
    return state == GLFW_PRESS;
}

bool Input::IsKeyReleased(Keycode key)
{
    auto* window = Application::Instance().GetWindow().GetNativeWindow();
    auto state = glfwGetKey(window, key);
    return state == GLFW_RELEASE;
}

bool Input::IsMousePressed(MouseCode button)
{
    auto* window = Application::Instance().GetWindow().GetNativeWindow();
    auto state = glfwGetMouseButton(window, button);
    return state == GLFW_PRESS || state == GLFW_REPEAT;
}

MousePosition Input::GetMousePosition()
{
    auto* window = Application::Instance().GetWindow().GetNativeWindow();
    double xPos, yPos;
    MousePosition pos;
    glfwGetCursorPos(window, &xPos, &yPos);
    pos.x_position = (float)xPos;
    pos.y_position = (float)yPos;
    return pos;
}

float Input::GetMouseX()
{
    return GetMousePosition().x_position;
}

float Input::GetMouseY()
{
    return GetMousePosition().y_position;
}
