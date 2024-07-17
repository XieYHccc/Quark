#include "pch.h"
#include "Platform/MacOS/InputManagerGLFW.h"
#include "Core/Window.h"

template <>
template <>
InputManager* MakeSingletonPtr<InputManager>::CreateSingleton()
{
    CORE_DEBUG_ASSERT(m_global == nullptr);

    m_global = new InputManagerGLFW();
    return m_global;
}

void InputManagerGLFW::Init()
{
    window_ = static_cast<GLFWwindow*>(Window::Instance()->GetNativeWindow());
    CORE_DEBUG_ASSERT(window_ != nullptr);

    double xpos, ypos;
    glfwGetCursorPos(window_, &xpos, &ypos);

    mousePosition_ = {(float)xpos, (float)ypos};
}

void InputManagerGLFW::Update()
{
    // handle events
    glfwPollEvents();

}

void InputManagerGLFW::Finalize()
{

}

void InputManagerGLFW::RecordKey(int key, int action)
{ 
    CORE_DEBUG_ASSERT(key > 0)
    keyMouseStatus_[key] = (KeyAction)action;
}

void InputManagerGLFW::RecordMousePosition(float xpos, float ypos)
{
    mousePosition_ = {xpos, ypos};
}
