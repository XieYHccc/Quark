#include "Application/Application.h"

#include <glad/glad.h>

#include "Foundation/KeyCodes.h"
#include "Events/EventManager.h"
#include "Events/ApplicationEvent.h"
#include "Application/Window/Input.h"
#include "Scene/SceneMngr.h"
#include "Render/Camera.h"


Application* Application::instance_ = nullptr;

Application::Application(const std::string& title, const std::string& root, int width, int height) 
{
    instance_ = this;
    root_ = root;
    running_ = true;
    fps_ = 0;
    frame_time_ = 0;

    // Initialize Camera
    Camera::global_camera.aspect = (float)width / height;
    Camera::global_camera.MovementSpeed = 30;
    Camera::global_camera.Yaw = -90.f;
    Camera::global_camera.Pitch = -30.f;
    
    // Initialize window
    WindowProps props;
    props.title = title;
    props.height = height;
    props.width = width;
    window_.Initialize(props);

    // Initialize run time modules
    EventManager::Instance().Initialize();
    SceneMngr::Instance().Initialize();

    // Register application callback functions
    EventManager::Instance().Subscribe<WindowCloseEvent>([this](const WindowCloseEvent& event) { OnWindowClose(event);});
}

Application::~Application() {
    EventManager::Instance().Finalize();
    SceneMngr::Instance().Finalize();
    window_.Finalize();
}

void Application::Run()
{
    while (running_) {
        // per-frame time logic
        float current_time = window_.GetFrameTime();
        float delta_time = current_time - frame_time_;
        frame_time_ = current_time;

        // Update each moudule (including processing inputs)
        Camera::global_camera.Update(delta_time);
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