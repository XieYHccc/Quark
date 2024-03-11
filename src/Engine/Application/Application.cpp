#include "./Application.h"

#include <memory>
#include <iostream>

#include <glm/glm.hpp>

#include "../Object/Object.h"
#include "../Render/Camera.h"
#include "../Scene/SceneMngr.h"
#include "../Object/Components/MeshRendererCmpt/MeshRenderCmpt.h"
#include "../physics/rigid_body_dynamics.h"
#include "../Input/InputMngr.h"
#include "../Input/KeyCode.h"
#include "../Object/Components/TransformCmpt/transform.h"
#include "../physics/collision_detection.h"

Camera Camera::global_camera = Camera(glm::vec3(0.0f, 5.0f, 20.0f));

Application::~Application() {
    Finalize();
}

void Application::Finalize() {

}

void Application::Initialize(const std::string& title, int width, int height) {
	title_ = title;
	width_ = width;
	height_ = height;
    delta_time_ = 0.0f;
    last_frame_ = 0.0f;

    
    // Initialize runtime modules
    InputMngr::Instance().Initialize();
    SceneMngr::Instance().Initialize();

    Camera::global_camera.aspect = (float)width / height;
    Camera::global_camera.MovementSpeed = 30;
    Camera::global_camera.Yaw = -90.f;
    Camera::global_camera.Pitch = -30.f;


    // Initialize and configure glfw window 
    if (!glfwInit())
        exit(EXIT_FAILURE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_SAMPLES, 4);

    window_ = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);

    if (!window_) {
        std::cerr << "Cannot create GLFW window.\n";
        glfwTerminate();
        exit(EXIT_FAILURE);
    }
    glfwMakeContextCurrent(window_);

    // enable v-sync
    glfwSwapInterval(1);

    // Initialize glad: load all OpenGL function pointers
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        exit(EXIT_FAILURE);
    }

    // debug: print GL and GLSL version
    std::cout << "GL     " << glGetString(GL_VERSION) << std::endl;
    std::cout << "GLSL   " << glGetString(GL_SHADING_LANGUAGE_VERSION) << std::endl;
    // call glGetError once to clear error queue
    glGetError();
    // detect highDPI framebuffer scaling and UI scaling
    int window_width, window_height, framebuffer_width, framebuffer_height;
    glfwGetWindowSize(window_, &window_width, &window_height);
    glfwGetFramebufferSize(window_, &framebuffer_width, &framebuffer_height);
    width_ = framebuffer_width;
    height_ = framebuffer_height;

    // Register glfw callbacks

    glfwSetKeyCallback(window_, [](GLFWwindow* window, int key, int scancode, int action, int mods)
    {
        InputMngr::Instance().RecordKey(key, action);
    });

    glfwSetCursorPosCallback(window_, [](GLFWwindow* window, double xpos, double ypos)
    {
        float xposstion = static_cast<float>(xpos);
        float yposition = static_cast<float>(ypos);

        if (InputMngr::Instance().IsFirstMouse())
        {
            InputMngr::Instance().RecordMousePosition(xposstion, yposition);
            InputMngr::Instance().SetFirstMouse(false);
        }

        glm::vec2 mouse_pos = InputMngr::Instance().GetMousePosition();
        float xoffset = xpos - mouse_pos.x;
        float yoffset = mouse_pos.y - ypos; // reversed since y-coordinates go from bottom to top

        InputMngr::Instance().RecordMousePosition(xposstion, yposition);

        Camera::global_camera.ProcessMouseMovement(xoffset, yoffset);
    });

    glfwSetFramebufferSizeCallback(window_, [](GLFWwindow* window, int width, int height)
    {
        Application& instance = *static_cast<Application*>(glfwGetWindowUserPointer(window));
        instance.width_ = width;
        instance.height_ = height;
        glViewport(0, 0, width, height);
    });

    // tell GLFW to capture our mouse
    // glfwSetInputMode(window_, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // Set window user pointer
    glfwSetWindowUserPointer(window_, reinterpret_cast<void*>(this));

    // Configure global OpenGl state
    glEnable(GL_DEPTH_TEST);

}

void Application::Run() {
    while (!glfwWindowShouldClose(window_)) {

        // per-frame time logic
        float currentFrame = static_cast<float>(glfwGetTime());
        delta_time_ = currentFrame - last_frame_;
        last_frame_ = currentFrame;

        if (InputMngr::Instance().GetKeyDown(KEY_CODE_W))
            Camera::global_camera.ProcessKeyboard(FORWARD, delta_time_);
        if (InputMngr::Instance().GetKeyDown(KEY_CODE_S))
            Camera::global_camera.ProcessKeyboard(BACKWARD, delta_time_);
        if (InputMngr::Instance().GetKeyDown(KEY_CODE_A))
            Camera::global_camera.ProcessKeyboard(LEFT, delta_time_);
        if (InputMngr::Instance().GetKeyDown(KEY_CODE_D))
            Camera::global_camera.ProcessKeyboard(RIGHT, delta_time_);
        if (InputMngr::Instance().GetKeyDown(KEY_CODE_K)) {
            auto bunny = SceneMngr::Instance().get_object("bunny");
            auto transform = bunny->get_component<Transform>();
            auto rigid_body = bunny->get_component<RigidBodyDynamic>();
            transform->set_position(glm::vec3(0.f, 1.f, 0.f));
            transform->set_rotation(glm::quat(1.f, 0.f, 0.f, 0.f));
            rigid_body->init_velocity();

        }
        if (InputMngr::Instance().GetKeyDown(KEY_CODE_L)) {
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
        // update each module
        Update();
        // render the scene
        Render();
        // swap buffers
        glfwSwapBuffers(window_);
        // handle events
        glfwPollEvents();
    }
}

void Application::Render() {

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

void Application::Update() {
    // update window state
    if (InputMngr::Instance().GetKeyDown(KEY_CODE_ESCAPE) || InputMngr::Instance().GetKeyDown(KEY_CODE_Q))
        exit(0);

    // update Input module
    InputMngr::Instance().Update();
}

