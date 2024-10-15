#pragma once
#include "Quark/Core/Base.h"
#include "Quark/Core/Timer.h"
#include "Quark/Core/TimeStep.h"
#include "Quark/Core/Window.h"
#include "Quark/Core/JobSystem.h"
#include "Quark/Events/ApplicationEvent.h"
#include "Quark/Graphic/Device.h"
#include "Quark/UI/UI.h"

namespace quark {

struct ApplicationSpecification 
{
    std::string title = "Quark Application";
    std::uint32_t width = 1200;
    std::uint32_t height = 800;
    bool isFullScreen = false;
    UiSpecification uiSpecs;
};  

class Application {
public:
    static Application& Get() { return *s_Instance; }
    
    Application(const ApplicationSpecification& specs);
    virtual ~Application();

    void Run();

    // Update Game Logic per frame
    virtual void OnUpdate(TimeStep ts) = 0;

    // Render per frame : Sync draw data with scene & All rendering cmd list recording here
    virtual void OnRender(TimeStep ts) = 0;

    // Prepare UI data
    virtual void OnImGuiUpdate() {};

    // Callback functions for events
    void OnWindowClose(const WindowCloseEvent& event);
    void OnWindowResize(const WindowResizeEvent& event);

    graphic::Device* GetGraphicDevice() { return m_GraphicDevice.get(); }
    JobSystem* GetJobSystem() { return m_JobSystem.get(); }

    Window* GetWindow() { return m_Window.get(); }

protected:
    struct ApplicationStatus
    {
        float fps = 0;
        bool isRunning = true;
        bool isMinimized = false;
        float lastFrameDuration = 0; // in seconds
    } m_Status;

    Timer m_Timer;

    Scope<graphic::Device> m_GraphicDevice;
    Scope<JobSystem> m_JobSystem;
    Scope<Window> m_Window;

private:
    static Application* s_Instance;
};

// To be defined in CLIENT
Application* CreateApplication();

}