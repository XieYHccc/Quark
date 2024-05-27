#pragma once

#include "Core/Timer.h"
#include "Events/ApplicationEvent.h"

class Application {

public:
    static Application& Instance() { return *singleton_; }
private:
    static Application* singleton_;
public:
    Application(const std::string& title, const std::string& root, int width, int height);
    virtual ~Application();
    Application(const Application&) = delete;
    const Application& operator=(const Application&) = delete;

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
    struct AppStatus 
    {
        f32 fps { 0 };
        bool isRunning { true };
        f64 lastFrameTime {0};

    };

    Timer m_Timer;
    AppStatus m_Status;

    // TODO: Support file system
    std::string m_Root;
};

// To be defined in CLIENT
Application* CreateApplication();