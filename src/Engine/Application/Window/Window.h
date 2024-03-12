#pragma once

#include <string>
#include <memory>

struct GLFWwindow;
struct GLFWmonitor;

struct WindowProps {

    std::string title;
    unsigned int width;
    unsigned int height;
    bool is_fullscreen;
    bool is_vsync;

    WindowProps(const std::string& title = "XEngine", unsigned int width = 1280, unsigned int height = 720, bool isFullScreen = false, bool isVSync = true)
        : title(title)
        , width(width)
        , height(height)
        , is_fullscreen(isFullScreen)
        , is_vsync(isVSync)
    {
    }
};

// Window Interface for Window Based Application
class Window {
public:
    Window() : window_(nullptr), monitor_(nullptr) {};
    ~Window();
    
    void Initialize(const WindowProps& props);
    void Finalize();

    void Update();

    unsigned int GetWidth() const { return props_.width; }
    unsigned int GetHeight() const { return props_.height; }
    float GetFrameTime() const;

    void SetVSync(bool enabled);
    bool IsVSync() const { return props_.is_vsync; }

    void SetFullScreen(bool enabled) { props_.is_fullscreen = enabled; }
    bool IsFullScreen() const { return props_.is_fullscreen; }

    GLFWwindow* GetNativeWindow() { return window_; }

private:
    GLFWwindow* window_;
    GLFWmonitor* monitor_;
    WindowProps props_;
};
