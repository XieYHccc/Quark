#include "Core/Application.h"

#include "Core/KeyMouseCodes.h"
#include "Core/Window.h"
#include "Core/Input.h"
#include "Events/EventManager.h"
#include "Events/ApplicationEvent.h"
#include "Asset/AssetManager.h"

#ifdef  USE_VULKAN_DRIVER
#include "Graphic/Vulkan/Device_Vulkan.h"
#endif

Application* Application::singleton_ = nullptr;

Application::Application(const AppInitSpecs& specs) 
{
    singleton_ = this;
    
    // Init logger
    Logger::Init();

    // Init event manager
    EventManager::Instance().Init();

    // Init window 
    Window::Create();
    Window::Instance()->Init(specs.title,specs.isFullScreen,  specs.width, specs.height);
    
    // Init input system
    Input::CreateSingleton();
    Input::Singleton()->Init();

    // Init graphic device
#ifdef  USE_VULKAN_DRIVER
    m_GraphicDevice = CreateScope<graphic::Device_Vulkan>();
    m_GraphicDevice->Init();
#endif

    // Init UI system
    UI::CreateSingleton();
    UI::Singleton()->Init(m_GraphicDevice.get(), specs.uiSpecs);

    // Register application callback functions
    EventManager::Instance().Subscribe<WindowCloseEvent>([this](const WindowCloseEvent& event) { OnWindowClose(event);});
}

Application::~Application() {

    // Destroy UI system
    UI::Singleton()->Finalize();
    UI::FreeSingleton();
    
#ifdef  USE_VULKAN_DRIVER
    m_GraphicDevice->ShutDown();
#endif

    // Destroy InputManager
    Input::Singleton()->Finalize();
    Input::FreeSingleton();

    // Destroy window
    Window::Instance()->Finalize();
    Window::Destroy();

    // destroy event manager
    EventManager::Instance().Finalize();
}

void Application::Run()
{
    while (m_Status.isRunning) {
        f64 start_frame = m_Timer.ElapsedMillis();

        // Poll events
        Input::Singleton()->Update();
        
        // TODO: Multithreading
        // Update each moudule (including processing inputs)
        Update(m_Status.lastFrameDuration);

        // Render Scene
        Render(m_Status.lastFrameDuration);

        // Dispatch events
        EventManager::Instance().DispatchEvents();

        m_Status.lastFrameDuration = m_Timer.ElapsedMillis() - start_frame;
        m_Status.fps = 1000.0f / m_Status.lastFrameDuration;
    }
}

void Application::OnWindowClose(const WindowCloseEvent& e)
{
    m_Status.isRunning = false;
}