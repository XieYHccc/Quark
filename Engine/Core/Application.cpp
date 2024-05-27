#include "Core/Application.h"

#include "Core/KeyCodes.h"
#include "Core/Window.h"
#include "Core/Input.h"
#include "Events/EventManager.h"
#include "Events/ApplicationEvent.h"
#include "Scene/SceneMngr.h"
#include "Renderer/Renderer.h"
#include "Asset/AssetManager.h"
#include "Rendering/RenderDevice.h"

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

    // Init SceneManager
    SceneMngr::Instance().Init();

    //Renderer::Instance().Init();
    RenderDevice::CreateSingleton();
    RenderDevice::Singleton().Init();


    // Register application callback functions
    EventManager::Instance().Subscribe<WindowCloseEvent>([this](const WindowCloseEvent& event) { OnWindowClose(event);});
}

Application::~Application() {
    
    SceneMngr::Instance().Finalize();

    asset::Mesh::ClearPool();
    asset::Sampler::ClearPool();
    asset::Texture::ClearPool();
    asset::Material::ClearPool();

    AssetManager::Instance().Finalize();
    
    //Renderer::Instance().Finalize();
    RenderDevice::Singleton().ShutDown();
    RenderDevice::FreeSingleton();
    // destroy window
    Window::Instance()->Finalize();
    Window::Destroy();

    // destroy event manager
    EventManager::Instance().Finalize();
}

void Application::Run()
{
    while (m_Status.isRunning) {
        m_Status.lastFrameTime = m_Timer.Elapsed();
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
    m_Status.isRunning = false;
}