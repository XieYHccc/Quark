#include "Application/Application.h"

#include "Core/KeyCodes.h"
#include "Core/Window.h"
#include "Core/Input.h"
#include "Events/EventManager.h"
#include "Events/ApplicationEvent.h"
#include "Scene/SceneMngr.h"
#include "Renderer/Renderer.h"
#include "Asset/AssetManager.h"

Application* Application::singleton_ = nullptr;

Application::Application(const std::string& title, const std::string& root, int width, int height) 
{
    singleton_ = this;
    root_ = root;
    running_ = true;
    fps_ = 0;
    frameTime_ = 0;
    deltaTime_ = 0;
    
    // Init logger
    Logger::Init();

    // Init run time modules
    EventManager::Instance().Init();

    // Init window
    Window::Create();
    Window::Instance()->Init(title,false,  width, height);

    // Init AssetManager
    AssetManager::Instance().Init();

    // Init SceneManager
    SceneMngr::Instance().Init();

#ifdef GRAPHIC_API_VULKAN
    Renderer::Instance().Init();
#endif

    // Register application callback functions
    EventManager::Instance().Subscribe<WindowCloseEvent>([this](const WindowCloseEvent& event) { OnWindowClose(event);});

    CORE_LOGI("Init Engine Successfully")
}

Application::~Application() {
    
    SceneMngr::Instance().Finalize();

    asset::Mesh::ClearPool();
    asset::Sampler::ClearPool();
    asset::Texture::ClearPool();
    asset::Material::ClearPool();

    AssetManager::Instance().Finalize();
    
#ifdef GRAPHIC_API_VULKAN
    Renderer::Instance().Finalize();
#endif 

    // destroy window
    Window::Instance()->Finalize();
    Window::Destroy();

    // destroy event manager
    EventManager::Instance().Finalize();
}

void Application::Run()
{
    while (running_) {

        // Update each moudule (including processing inputs)
        Update();

        // Render Scene
        Render();

        // Swap buffers
        Window::Instance()->Update();

        EventManager::Instance().DispatchEvents();
    }
}

void Application::OnWindowClose(const WindowCloseEvent& e)
{
    running_ = false;
}