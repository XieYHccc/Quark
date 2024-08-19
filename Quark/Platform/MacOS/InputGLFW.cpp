#include "Quark/qkpch.h"
#include "Quark/Platform/MacOS/InputGLFW.h"
#include "Quark/Core/Window.h"

namespace quark {

template <>
template <>
Input* util::MakeSingletonPtr<Input>::CreateSingleton()
{
    CORE_DEBUG_ASSERT(m_global == nullptr);

    m_global = new InputGLFW();
    return m_global;
}

void InputGLFW::Init()
{
    window_ = static_cast<GLFWwindow*>(Window::Instance()->GetNativeWindow());
    CORE_DEBUG_ASSERT(window_ != nullptr);

    double xpos, ypos;
    glfwGetCursorPos(window_, &xpos, &ypos);

    mousePosition_ = {(float)xpos, (float)ypos};
}

void InputGLFW::Update()
{
    // handle events
    glfwPollEvents();

}

void InputGLFW::Finalize()
{

}

void InputGLFW::RecordKey(int key, int action)
{ 
    CORE_DEBUG_ASSERT(key >= 0)
    keyMouseStatus_[key] = (KeyAction)action;
}

void InputGLFW::RecordMousePosition(float xpos, float ypos)
{
    mousePosition_ = {xpos, ypos};
}

}