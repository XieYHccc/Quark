#pragma once
#include <GLFW/glfw3.h>
#include "Quark/Core/Window.h"
#include "Quark/Platform/MacOS/InputGLFW.h"

namespace quark {

class WindowGLFW final: public Window {
public:
    WindowGLFW(const WindowSpecification& spec);
    virtual ~WindowGLFW() {};

    void Init() override final;
    void ShutDown() override final;

    u32 GetFrambufferWidth() const override final;
    u32 GetFrambufferHeight() const override final;
    u32 GetMonitorWidth() const override final { return m_monitorWidth; }
    u32 GetMonitorHeight() const override final { return m_monitorHeight; }

    void* GetNativeWindow() override final { return m_glfwWindow; }
    
private:
    GLFWwindow* m_glfwWindow = nullptr;
    GLFWmonitor* m_glfwMonitor = nullptr;

    uint32_t m_monitorWidth;
    uint32_t m_monitorHeight;
};

}