#include "Core/Application.h"

#include "Core/KeyMouseCodes.h"
#include "Core/Window.h"
#include "Core/Input.h"
#include "Events/EventManager.h"
#include "Events/ApplicationEvent.h"
#include "Asset/AssetManager.h"
#include "UI/UI.h"

#ifdef  USE_VULKAN_DRIVER
#include "Graphic/Vulkan/Device_Vulkan.h"
#endif

Application* Application::singleton_ = nullptr;

Application::Application(const std::string& title, const std::string& root, int width, int height) 
{
    singleton_ = this;
    m_Root = root;
    
    
    // Init logger
    Logger::Init();

    // Init event manager
    EventManager::Instance().Init();

    // Init window 
    Window::Create();
    Window::Instance()->Init(title,false,  width, height);
    
    // Init input system
    Input::CreateSingleton();
    Input::Singleton()->Init();

    // Init asset manager
    AssetManager::Instance().Init();

    // Init graphic device
#ifdef  USE_VULKAN_DRIVER
    m_GraphicDevice = CreateScope<graphic::Device_Vulkan>();
    m_GraphicDevice->Init();
#endif

    // Init UI system
    UI::CreateSingleton();
    UI::Singleton()->Init(m_GraphicDevice.get());

    // Register application callback functions
    EventManager::Instance().Subscribe<WindowCloseEvent>([this](const WindowCloseEvent& event) { OnWindowClose(event);});
}

Application::~Application() {

    AssetManager::Instance().Finalize();
    
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