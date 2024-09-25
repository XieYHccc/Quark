#include "Quark/qkpch.h"
#include "Quark/Platform/MacOS/InputGLFW.h"
#include "Quark/Core/Application.h"

namespace quark {

template <>
template <>
Input* util::MakeSingletonPtr<Input>::CreateSingleton()
{
    QK_CORE_ASSERT(m_global == nullptr);

    m_global = new InputGLFW();
    return m_global;
}

void InputGLFW::Init()
{
    window_ = static_cast<GLFWwindow*>(Application::Get().GetWindow()->GetNativeWindow());
    QK_CORE_ASSERT(window_ != nullptr);

    double xpos, ypos;
    glfwGetCursorPos(window_, &xpos, &ypos);

    mousePosition_ = {(float)xpos, (float)ypos};
}

void InputGLFW::OnUpdate()
{
    // handle events
    glfwPollEvents();

}

void InputGLFW::Finalize()
{

}

void InputGLFW::RecordKey(int key, int action)
{ 
    QK_CORE_ASSERT(key >= 0)
    keyMouseStatus_[key] = (KeyAction)action;
}

void InputGLFW::RecordMousePosition(float xpos, float ypos)
{
    mousePosition_ = {xpos, ypos};
}

}