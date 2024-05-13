#pragma once
#include <GLFW/glfw3.h>
#include "Core/Window.h"

class WindowGLFW : public Window {
public:
    WindowGLFW();
    ~WindowGLFW() {};

    void Init(const std::string& title, bool is_fullscreen, u32 width, u32 height) override;
    void Finalize() override;

    void Update() override;

    uint32_t GetWidth() const override { return width_; }
    uint32_t GetHeight() const override { return height_; }

    void* GetNativeWindow() override { return window_; }
    
private:
    GLFWwindow* window_;
    GLFWmonitor* monitor_;
    bool isFullscreen_;
    std::string title_;
    u32 width_;
    u32 height_;
};