#include "Core/Application.h"

#include "Core/KeyMouseCodes.h"
#include "Core/Window.h"
#include "Core/InputManager.h"
#include "Events/EventManager.h"
#include "Events/ApplicationEvent.h"
#include "Asset/AssetManager.h"

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
    
    // Init input manager
    InputManager::CreateSingleton();
    InputManager::Singleton()->Init();

    // Init asset manager
    AssetManager::Instance().Init();

    // Init graphic device
#ifdef  USE_VULKAN_DRIVER
    m_GraphicDevice = CreateScope<graphic::Device_Vulkan>();
    m_GraphicDevice->Init();
#endif


    // Register application callback functions
    EventManager::Instance().Subscribe<WindowCloseEvent>([this](const WindowCloseEvent& event) { OnWindowClose(event);});
}

Application::~Application() {

    AssetManager::Instance().Finalize();
    
#ifdef  USE_VULKAN_DRIVER
    m_GraphicDevice->ShutDown();
#endif

    // Destroy InputManager
    InputManager::Singleton()->Finalize();
    InputManager::FreeSingleton();

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
    }
}

void Application::OnWindowClose(const WindowCloseEvent& e)
{
    m_Status.isRunning = false;
}