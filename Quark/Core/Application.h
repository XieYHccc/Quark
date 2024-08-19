#pragma once
#include "Quark/Core/Base.h"
#include "Quark/Core/Timer.h"
#include "Quark/Core/TimeStep.h"
#include "Quark/Core/Window.h"
#include "Quark/Events/ApplicationEvent.h"
#include "Quark/Graphic/Device.h"
#include "Quark/UI/UI.h"

namespace quark {

struct AppInitSpecs {
    std::string title = "Quark Application";
    std::uint32_t width = 1200;
    std::uint32_t height = 800;
    bool isFullScreen = false;
    UiInitSpecs uiSpecs;
};  

class Application {
public:
    static Application& Get() { return *s_Instance; }
    
    Application(const AppInitSpecs& specs);
    virtual ~Application();
    Application(const Application&) = delete;
    const Application& operator=(const Application&) = delete;

    void Run();

    graphic::Device* GetGraphicDevice() { return m_GraphicDevice.get();}

private:
    // Update Game Logic per frame
    virtual void OnUpdate(TimeStep ts) = 0;

    // Render per frame : Sync draw data with scene & All rendering cmd list recording here
    virtual void OnRender(TimeStep ts) = 0;

    // Prepare UI data
    virtual void OnUpdateImGui() {};

    // Callback functions for events
    void OnWindowClose(const WindowCloseEvent& event);
    void OnWindowResize(const WindowResizeEvent& event) {}

protected:
    struct AppStatus 
    {
        f32 fps { 0 };
        bool isRunning { true };
        f64 lastFrameDuration { 0 };
    };

    Timer m_Timer;
    AppStatus m_Status;
    Scope<graphic::Device> m_GraphicDevice;
    Scope<Window> m_Window;
    
private:
    static Application* s_Instance;
};

// To be defined in CLIENT
Application* CreateApplication();

}