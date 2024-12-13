#include "Quark/qkpch.h"
#include "Quark/Core/Application.h"
#include "Quark/Core/Input.h"
#include "Quark/Events/EventManager.h"
#include "Quark/Events/ApplicationEvent.h"
#include "Quark/Asset/AssetManager.h"
#include "Quark/Render/RenderSystem.h"

#ifdef USE_VULKAN_DRIVER
#include "Quark/RHI/Vulkan/Device_Vulkan.h"
#endif

#if defined(QK_PLATFORM_WINDOWS) || defined(QK_PLATFORM_MACOS)
#include "Quark/Platform/MacOS/WindowGLFW.h"
#endif

#include <nfd.hpp>

namespace quark {

Application* Application::s_instance = nullptr;

Application::Application(const ApplicationSpecification& specs) 
{
    s_instance = this;

    if (!specs.workingDirectory.empty())
        std::filesystem::current_path(specs.workingDirectory);

    Logger::Init();

    // Init Job System
    m_jobSystem = CreateRef<JobSystem>();

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
        m_window = CreateScope<WindowGLFW>(windowSpec);
        m_window->Init();
#endif
        NFD::Init();

        QK_CORE_LOGI_TAG("Core", "Window created");
    }

#ifdef USE_VULKAN_DRIVER
    m_graphicDevice = CreateRef<rhi::Device_Vulkan>();
    m_graphicDevice->Init();
#endif

    // Init Graphic Device and Renderer
    JobSystem::Counter counter;
    m_jobSystem->Execute([this]()
    {
// #ifdef USE_VULKAN_DRIVER
//         m_GraphicDevice = CreateScope<rhi::Device_Vulkan>();
//         m_GraphicDevice->Init();
// #endif
        RenderSystem::CreateSingleton(m_graphicDevice);
    }, &counter);

    // Init Asset system
    m_jobSystem->Execute([this, &counter]() 
    {
        m_jobSystem->Wait(&counter, 1);
        AssetManager::CreateSingleton(); 
    });

    // Init UI system
    m_jobSystem->Execute([this, &specs, &counter]() 
    {
        m_jobSystem->Wait(&counter, 1);
        UI::CreateSingleton();
        UI::Get()->Init(m_graphicDevice.get(), specs.uiSpecs);
    });

    m_jobSystem->Wait(&counter, 1);

    // Register application callback functions
    EventManager::Get().Subscribe<WindowCloseEvent>([this](const WindowCloseEvent& event) { OnWindowClose(event);});
    EventManager::Get().Subscribe<WindowResizeEvent>([this](const WindowResizeEvent& event) { OnWindowResize(event); });
}

Application::~Application() {

    UI::Get()->Finalize();
    UI::FreeSingleton();

    AssetManager::FreeSingleton();

    RenderSystem::FreeSingleton();

    m_graphicDevice->ShutDown();
    m_graphicDevice.reset();

    // Destroy InputManager
    Input::Get()->Finalize();
    Input::FreeSingleton();

    // Destroy window
    m_window->ShutDown();
    m_window.reset();

    EventManager::FreeSingleton();

    m_jobSystem.reset();

    Logger::ShutDown();
}

void Application::Run()
{
    while (m_status.isRunning) 
    {
        float start_frame = m_timer.ElapsedSeconds();

        // Poll events
        Input::Get()->OnUpdate();
        
        if (!m_status.isMinimized)
        {
            // TODO: Multithreading
            OnUpdate(m_status.lastFrameDuration);

            OnImGuiUpdate();

            OnRender(m_status.lastFrameDuration);
        }

        // Dispatch events
        EventManager::Get().DispatchEvents();

        m_status.lastFrameDuration = m_timer.ElapsedSeconds() - start_frame;
        m_status.fps = 1.f / m_status.lastFrameDuration;
    }
}

void Application::OnWindowClose(const WindowCloseEvent& e)
{
    m_status.isRunning = false;
}

void Application::OnWindowResize(const WindowResizeEvent& event)
{
    if (event.width == 0 || event.height == 0)
    {
        m_status.isMinimized = true;
        return;
    }

    m_status.isMinimized = false;
}

}