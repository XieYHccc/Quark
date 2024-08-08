#pragma once
#include <GLFW/glfw3.h>
#include "Quark/Core/Window.h"
#include "Quark/Platform/MacOS/InputGLFW.h"

namespace quark {

class WindowGLFW final: public Window {
public:
    WindowGLFW();
    ~WindowGLFW() {};

    void Init(const std::string& title, bool is_fullscreen, u32 width, u32 height) override final;
    void Finalize() override final;

    u32 GetWidth() const override final { return width_; }
    u32 GetHeight() const override final { return height_; }
    u32 GetFrambufferWidth() const override final;
    u32 GetFrambufferHeight() const override final;
    u32 GetMonitorWidth() const override final { return monitorWidth_; }
    u32 GetMonitorHeight() const override final { return monitorHeight_; }

    void* GetNativeWindow() override final { return window_; }
    
private:
    GLFWwindow* window_ = nullptr;
    GLFWmonitor* monitor_ = nullptr;

    bool isFullscreen_;
    std::string title_;
    u32 width_;
    u32 height_;
    u32 monitorWidth_;
    u32 monitorHeight_;
};

}