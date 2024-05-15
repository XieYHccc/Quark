#pragma once

#include "pch.h"
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
    // Application status
    float fps_;

    // TODO: SUPPORT PER-FRAME LOGIC
    float frameTime_;
    float deltaTime_;
    bool running_;
    std::string root_;
};

// To be defined in CLIENT
Application* CreateApplication();