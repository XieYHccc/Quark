#pragma once

#include <string>
#include "Application/Window/Window.h"
#include "Events/ApplicationEvent.h"

class Application {
public:
    Application(const std::string& title, const std::string& root, int width, int height);
    virtual ~Application();
    Application(const Application&) = delete;
    const Application& operator=(const Application&) = delete;

    static Application& Instance() { return *instance_; }
    Window& GetWindow() { return window_; }

    void Run();

private:
    // Update some Moudule Per Frame
    virtual void Update();
    // Render Per Frame
    virtual void Render();

    // Callback functions for events
    void OnWindowClose(const WindowCloseEvent& event);
    void OnWindowResize(const WindowResizeEvent& event) {}

private:
    static Application* instance_;
    
    // Application status
    float fps_;
    float frame_time_;
    float delta_time_;
    bool running_;

    Window window_;
    std::string root_;

};