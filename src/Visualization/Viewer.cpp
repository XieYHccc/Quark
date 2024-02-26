#include "Viewer.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "../components/object.h"
#include "../components/mesh_displayer.h"

Camera Camera::global_camera = Camera(glm::vec3(0.0f, 0.0f, 10.0f));
Viewer::Viewer(const char* title, int width, int height)
:Window(title, width, height){

    lastX_ = 0.f;
    lastY_ = 0.f;
    float deltaTime = 0.0f;
    float lastFrame = 0.0f;
    first_mouse_ = true;
    Camera::global_camera.aspect = (float)width / height;
    
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

    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // view/projection transformations
    glm::mat4 projection = glm::perspective(glm::radians(Camera::global_camera.Zoom), (float)width() / height(), 0.1f, 100.0f);
    glm::mat4 view = Camera::global_camera.GetViewMatrix();

    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0f)); // translate it down so it's at the center of the scene
    model = glm::scale(model, glm::vec3(1.0f, 1.0f, 1.0f));	// it's a bit too big for our scene, so scale it down
    glm::mat4 mvp = projection * view * model;
    for (auto obj : Object::object_list_) {
        auto component_mesh_displayer = obj->get_component("MeshDisplayer");
        auto mesh_diplayer = dynamic_cast<MeshDisplayer*>(component_mesh_displayer);
        mesh_diplayer->render();
    }

}