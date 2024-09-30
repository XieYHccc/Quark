#include "Quark/qkpch.h"

#include <nfd.hpp>

#include "Quark/Core/Application.h"
#include "Quark/Core/Input.h"
#include "Quark/Events/EventManager.h"
#include "Quark/Events/ApplicationEvent.h"
#include "Quark/Asset/AssetManager.h"
#include "Quark/Renderer/Renderer.h"
#include "Quark/Renderer/ShaderLibrary.h"

#ifdef USE_VULKAN_DRIVER
#include "Quark/Graphic/Vulkan/Device_Vulkan.h"
#endif

#if defined(QK_PLATFORM_WINDOWS) || defined(QK_PLATFORM_MACOS)
#include "Quark/Platform/MacOS/WindowGLFW.h"
#endif

namespace quark {

Application* Application::s_Instance = nullptr;

Application::Application(const ApplicationSpecification& specs) 
{
    s_Instance = this;

    Logger::Init();

    // Init Job System
    m_JobSystem = CreateScope<JobSystem>();

    // Init Event Manager
    EventManager::CreateSingleton();

    // Init Input System
    Input::CreateSingleton();
    Input::Get()->Init();

    // Create Window
    {
        WindowSpecification windowSpec;
        windowSpec.width = specs.width;
        windowSpec.height = specs.height;
        windowSpec.title = specs.title;
        windowSpec.is_fullscreen = specs.isFullScreen;

#if defined(QK_PLATFORM_WINDOWS) || defined(QK_PLATFORM_MACOS)
        m_Window = CreateScope<WindowGLFW>(windowSpec);
        m_Window->Init();
#endif
        NFD::Init();

        QK_CORE_LOGI_TAG("Core", "Window created");
    }

#ifdef USE_VULKAN_DRIVER
    m_GraphicDevice = CreateScope<graphic::Device_Vulkan>();
    m_GraphicDevice->Init();
#endif

    // Init Graphic Device and Renderer
    JobSystem::Counter counter;
    m_JobSystem->Execute([this]()
    {
// #ifdef USE_VULKAN_DRIVER
//         m_GraphicDevice = CreateScope<graphic::Device_Vulkan>();
//         m_GraphicDevice->Init();
// #endif
        GpuResourceManager::CreateSingleton();
        GpuResourceManager::Get().Init();
    }, &counter);

    // Init Asset system
    m_JobSystem->Execute([this, &counter]() 
    {
        m_JobSystem->Wait(&counter, 1);
        AssetManager::CreateSingleton(); 
    });

    // Init UI system
    m_JobSystem->Execute([this, &specs, &counter]() 
    {
        m_JobSystem->Wait(&counter, 1);
        UI::CreateSingleton();
        UI::Get()->Init(m_GraphicDevice.get(), specs.uiSpecs);
    });

    m_JobSystem->Wait(&counter, 1);

    // Register application callback functions
    EventManager::Get().Subscribe<WindowCloseEvent>([this](const WindowCloseEvent& event) { OnWindowClose(event);});
    EventManager::Get().Subscribe<WindowResizeEvent>([this](const WindowResizeEvent& event) { OnWindowResize(event); });
}

Application::~Application() {

    UI::Get()->Finalize();
    UI::FreeSingleton();

    AssetManager::FreeSingleton();

    Renderer::Get().Shutdown();
    Renderer::FreeSingleton();

    m_GraphicDevice->ShutDown();
    m_GraphicDevice.reset();

    // Destroy InputManager
    Input::Get()->Finalize();
    Input::FreeSingleton();

    // Destroy window
    m_Window->ShutDown();
    m_Window.reset();

    EventManager::FreeSingleton();

    m_JobSystem.reset();

    Logger::ShutDown();
}

void Application::Run()
{
    while (m_Status.isRunning) 
    {
        f64 start_frame = m_Timer.ElapsedSeconds();

        // Poll events
        Input::Get()->OnUpdate();
        
        if (!m_Status.isMinimized)
        {
            // TODO: Multithreading
            OnUpdate(m_Status.lastFrameDuration);

            OnImGuiUpdate();

            OnRender(m_Status.lastFrameDuration);
        }

        // Dispatch events
        EventManager::Get().DispatchEvents();

        m_Status.lastFrameDuration = m_Timer.ElapsedSeconds() - start_frame;
        m_Status.fps = 1.f / m_Status.lastFrameDuration;
    }
}

void Application::OnWindowClose(const WindowCloseEvent& e)
{
    m_Status.isRunning = false;
}

void Application::OnWindowResize(const WindowResizeEvent& event)
{
    if (event.width == 0 || event.height == 0)
    {
        m_Status.isMinimized = true;
        return;
    }

    m_Status.isMinimized = false;
}

}