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
    // Update some modules per frame
    virtual void Update() = 0;
    // Render per frame
    virtual void Render() = 0;

    // Callback functions for events
    void OnWindowClose(const WindowCloseEvent& event);
    void OnWindowResize(const WindowResizeEvent& event) {}

protected:
    // Application status
    float fps_;
    float frame_time_;
    bool running_;

    std::string root_; // root directory

private:
    static Application* instance_;
    Window window_;

};

// To be defined in CLIENT
Application* CreateApplication();