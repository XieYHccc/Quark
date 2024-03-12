#include "Application/Application.h"

#include <glad/glad.h>

#include "Foundation/KeyCodes.h"
#include "Events/EventManager.h"
#include "Events/ApplicationEvent.h"
#include "Application/Input.h"
#include "Scene/SceneMngr.h"
#include "../Object/Components/MeshRendererCmpt/MeshRenderCmpt.h"
#include "../Object/Components/TransformCmpt/transform.h"
#include "../physics/rigid_body_dynamics.h"
#include "physics/collision_detection.h"
#include "Render/Camera.h"


Camera Camera::global_camera = Camera(glm::vec3(0.0f, 5.0f, 20.0f));
Application* Application::instance_ = nullptr;

Application::Application(const std::string& title, const std::string& root, int width, int height) 
{
    instance_ = this;
    root_ = root;
    running_ = true;
    fps_ = 0;
    frame_time_ = 0;
    delta_time_ = 0;

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
        delta_time_ = current_time - frame_time_;
        frame_time_ = current_time;

        // Update some moudules
        Update();

        // Render Scene
        Render();

        // swap buffers
        window_.Update();

        EventManager::Instance().DispatchEvents();
    }
}

void Application::Update()
{
    if (Input::IsKeyPressed(W))
        Camera::global_camera.ProcessKeyboard(FORWARD, delta_time_);
    if (Input::IsKeyPressed(S))
        Camera::global_camera.ProcessKeyboard(BACKWARD, delta_time_);
    if (Input::IsKeyPressed(A))
        Camera::global_camera.ProcessKeyboard(LEFT, delta_time_);
    if (Input::IsKeyPressed(D))
        Camera::global_camera.ProcessKeyboard(RIGHT, delta_time_);
    if (Input::IsKeyPressed(K)) {
        auto bunny = SceneMngr::Instance().get_object("bunny");
        auto transform = bunny->get_component<Transform>();
        auto rigid_body = bunny->get_component<RigidBodyDynamic>();
        transform->set_position(glm::vec3(0.f, 1.f, 0.f));
        transform->set_rotation(glm::quat(1.f, 0.f, 0.f, 0.f));
        rigid_body->init_velocity();

    }
    if (Input::IsKeyPressed(L)) {
        auto bunny = SceneMngr::Instance().get_object("bunny");
        auto transform = bunny->get_component<Transform>();
        auto rigid_body = bunny->get_component<RigidBodyDynamic>();
        rigid_body->init_velocity(glm::vec3(0.f, 3.f, -6.f));
        rigid_body->set_lauched(true);
    }
    auto bunny = SceneMngr::Instance().get_object("bunny");
    auto wall = SceneMngr::Instance().get_object("wall");
    auto gridbox = SceneMngr::Instance().get_object("gridbox");
    auto mesh_collider = bunny->get_component<MeshCollider>();
    auto wall_plane_collider = wall->get_component<PlaneCollider>();
    auto ground_plane_collider = gridbox->get_component<PlaneCollider>();
    check_collision(wall_plane_collider, mesh_collider);
    check_collision(ground_plane_collider, mesh_collider);
}

void Application::Render() 
{
    glClearColor(0.36f, 0.36f, 0.36f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    for (auto& obj : SceneMngr::Instance().object_map) {
        auto mesh_diplayer = obj.second->get_component<MeshRendererCmpt>();
        auto rigid_body = obj.second->get_component<RigidBodyDynamic>();
        if (mesh_diplayer == nullptr)
            continue;

        if (rigid_body != nullptr) {
            rigid_body->update();
        }

        mesh_diplayer->render();
    }
}

void Application::OnWindowClose(const WindowCloseEvent& e)
{
    running_ = false;
}