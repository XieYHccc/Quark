#pragma once
#include <GLFW/glfw3.h>
#include "Core/Window.h"
#include "Platform/MacOS/InputGLFW.h"

class WindowGLFW : public Window {
public:
    WindowGLFW();
    ~WindowGLFW() {};

    virtual void Init(const std::string& title, bool is_fullscreen, u32 width, u32 height) override final;
    virtual void Finalize() override final;

    virtual u32 GetWidth() const override final { return width_; }
    virtual u32 GetHeight() const override final { return height_; }
    virtual u32 GetFrambufferWidth() const override final;
    virtual u32 GetFrambufferHeight() const override final;
    virtual void* GetNativeWindow() override final { return window_; }
    
private:
    GLFWwindow* window_ = nullptr;
    GLFWmonitor* monitor_ = nullptr;

    bool isFullscreen_;
    std::string title_;
    u32 width_;
    u32 height_;
};