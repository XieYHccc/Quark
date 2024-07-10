#include "Core/Application.h"

#include "Core/KeyCodes.h"
#include "Core/Window.h"
#include "Core/Input.h"
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

    // Init run time modules
    EventManager::Instance().Init();

    // Init window
    Window::Create();
    Window::Instance()->Init(title,false,  width, height);

    // Init AssetManager
    AssetManager::Instance().Init();

    // Init Graphic Device
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

    // destroy window
    Window::Instance()->Finalize();
    Window::Destroy();

    // destroy event manager
    EventManager::Instance().Finalize();
}

void Application::Run()
{
    while (m_Status.isRunning) {
        auto start = std::chrono::system_clock::now();

        m_Status.lastFrameTime = m_Timer.Elapsed();
        // Update each moudule (including processing inputs)
        Update(m_Timer.Elapsed() - m_Status.lastFrameTime);

        // Render Scene
        Render(m_Timer.Elapsed() - m_Status.lastFrameTime);

        // Swap buffers
        Window::Instance()->Update();

        EventManager::Instance().DispatchEvents();

        auto end = std::chrono::system_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        // CORE_LOGI("Frame time : {} ms", elapsed.count() / 1000.f)
    }
}

void Application::OnWindowClose(const WindowCloseEvent& e)
{
    m_Status.isRunning = false;
}