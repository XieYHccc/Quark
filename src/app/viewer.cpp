#include "viewer.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "../basic/object.h"
#include "../basic/transform.h"
#include "../render/components/mesh_displayer.h"
#include "../physics/rigid_body_dynamics.h"
#include "../physics/collision_detection.h"
#include "../scene/scene.h"

// global variables
Camera Camera::global_camera = Camera(glm::vec3(0.0f, 5.0f, 5.0f));

Viewer::Viewer(const char* title, int width, int height)
    :Window(title, width, height) {

    lastX_ = 0.f;
    lastY_ = 0.f;
    float deltaTime = 0.0f;
    float lastFrame = 0.0f;
    first_mouse_ = true;
    Camera::global_camera.aspect = (float)width / height;
    Camera::global_camera.MovementSpeed = 30;
    Camera::global_camera.Yaw = -90.f;
    Camera::global_camera.Pitch = -30.f;
}

Viewer::~Viewer() {

}
void Viewer::process_input(GLFWwindow* window) {
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        Camera::global_camera.ProcessKeyboard(FORWARD, deltaTime_);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        Camera::global_camera.ProcessKeyboard(BACKWARD, deltaTime_);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        Camera::global_camera.ProcessKeyboard(LEFT, deltaTime_);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        Camera::global_camera.ProcessKeyboard(RIGHT, deltaTime_);

    if (glfwGetKey(window, GLFW_KEY_K) == GLFW_PRESS) {
        auto bunny = Scene::Instance().get_object("bunny");
        auto transform = dynamic_cast<Transform*>(bunny->get_component("Transform"));
        auto rigid_body = dynamic_cast<RigidBodyDynamic*>(bunny->get_component("RigidBodyDynamic"));
        transform->set_position(glm::vec3(0.f, 1.f, 0.f));
        transform->set_rotation(glm::quat(1.f, 0.f, 0.f, 0.f));
        rigid_body->init_velocity();

    }

    if (glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS) {
        auto bunny = Scene::Instance().get_object("bunny");
        auto transform = dynamic_cast<Transform*>(bunny->get_component("Transform"));
        auto rigid_body = dynamic_cast<RigidBodyDynamic*>(bunny->get_component("RigidBodyDynamic"));
        rigid_body->init_velocity(glm::vec3(0.f, 3.f, -6.f));
        rigid_body->set_lauched(true);
    }
        
}

void Viewer::motion(double xposIn, double yposIn) {
    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);

    if (first_mouse_)
    {
        lastX_ = xpos;
        lastY_ = ypos;
        first_mouse_ = false;
    }

    float xoffset = xpos - lastX_;
    float yoffset = lastY_ - ypos; // reversed since y-coordinates go from bottom to top

    lastX_ = xpos;
    lastY_ = ypos;

    Camera::global_camera.ProcessMouseMovement(xoffset, yoffset);
}

void Viewer::render() {
    // per-frame time logic
    float currentFrame;
    frame_time(currentFrame);
    deltaTime_ = currentFrame - lastFrame_;
    lastFrame_ = currentFrame;

    glClearColor(0.36f, 0.36f, 0.36f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    auto bunny = Scene::Instance().get_object("bunny");
    auto wall  = Scene::Instance().get_object("wall");
    auto gridbox = Scene::Instance().get_object("grid box");
    auto mesh_collider = dynamic_cast<MeshCollider*>(bunny->get_component("MeshCollider"));
    auto wall_plane_collider = dynamic_cast<PlaneCollider*>(wall->get_component("PlaneCollider"));
    auto ground_plane_collider = dynamic_cast<PlaneCollider*>(gridbox->get_component("PlaneCollider"));
    check_collision(wall_plane_collider, mesh_collider);
    check_collision(ground_plane_collider, mesh_collider);

    for (auto& obj : Scene::Instance().object_map) {
        auto mesh_diplayer = dynamic_cast<MeshDisplayer*>(obj.second->get_component("MeshDisplayer"));
        auto rigid_body = dynamic_cast<RigidBodyDynamic*>(obj.second->get_component("RigidBodyDynamic"));
        if (mesh_diplayer == nullptr)
            continue;

        if (rigid_body != nullptr) {
            rigid_body->update();
        }

        mesh_diplayer->render();
    }

}