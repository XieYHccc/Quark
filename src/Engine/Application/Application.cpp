#include "Application/Application.h"

#include <glad/glad.h>

#include "Foundation/KeyCodes.h"
#include "Events/EventManager.h"
#include "Events/ApplicationEvent.h"
#include "Application/Window/Input.h"
#include "Scene/SceneMngr.h"
#include "Graphics/Vulkan/RendererVulkan.h"


Application* Application::instance_ = nullptr;

Application::Application(const std::string& title, const std::string& root, int width, int height) 
{
    instance_ = this;
    root_ = root;
    running_ = true;
    fps_ = 0;
    frameTime_ = 0;
    deltaTime_ = 0;
    
    // Initialize window
    WindowProps props;
    props.title = title;
    props.height = height;
    props.width = width;
    window_.Initialize(props);

    // Initialize run time modules
    EventManager::Instance().Initialize();

#ifdef GRAPHIC_API_VULKAN
    RendererVulkan::Creat();
    RendererVulkan::GetInstance()->Initialize();
#endif

    SceneMngr::Instance().Initialize();

    // Register application callback functions
    EventManager::Instance().Subscribe<WindowCloseEvent>([this](const WindowCloseEvent& event) { OnWindowClose(event);});
}

Application::~Application() {
    
    SceneMngr::Instance().Finalize();

#ifdef GRAPHIC_API_VULKAN
    RendererVulkan::GetInstance()->Finalize();
#endif 

    EventManager::Instance().Finalize();
    window_.Finalize();
}

void Application::Run()
{
    while (running_) {
        // per-frame time logic
        float current_time = window_.GetFrameTime();
        deltaTime_ = current_time - frameTime_;
        frameTime_ = current_time;

        // Update each moudule (including processing inputs)
        Update();

        // Render Scene
        Render();

        // swap buffers
        window_.Update();

        EventManager::Instance().DispatchEvents();
    }
}

void Application::OnWindowClose(const WindowCloseEvent& e)
{
    running_ = false;
}