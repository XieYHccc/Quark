#include "mesh_displayer.h"

#include <rttr/registration.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/transform2.hpp>
#include <glm/gtx/euler_angles.hpp>

#include "object.h"
#include "mesh_filter.h"
#include "transform.h"
#include "../Visualization/Camera.h"
using namespace rttr;
RTTR_REGISTRATION
{
    registration::class_<MeshDisplayer>("MeshDisplayer")
            .constructor<>()(rttr::policy::ctor::as_raw_ptr);
}
MeshDisplayer::MeshDisplayer() : renderer_() {}

void MeshDisplayer::render() {
    if (!renderer_.valid_vao()) {
        auto component_mesh_filter = get_object()->get_component("MeshFilter");
        auto mesh_filter = dynamic_cast<MeshFilter*>(component_mesh_filter);
        if (!mesh_filter) { return; }
        renderer_.setup_vao(mesh_filter->mesh());
    }

    // Calculate model matrix
    auto component_transform = get_object()->get_component("Transform");
    auto transform = dynamic_cast<Transform*>(component_transform);
    if (!transform) { return; }
    glm::mat4 trans = glm::translate(transform->get_position());
    auto rotation=transform->get_rotation();
    glm::mat4 eulerAngleYXZ = glm::eulerAngleYXZ(glm::radians(rotation.y), glm::radians(rotation.x), glm::radians(rotation.z));
    glm::mat4 scale = glm::scale(transform->get_scale());
    glm::mat4 model = trans * scale * eulerAngleYXZ;
    renderer_.set_model_matrix(model);
    // Calculate view matrix
    Camera& camera = Camera::global_camera;
    glm::mat4 view = camera.GetViewMatrix();
    renderer_.set_view_matrix(view);
    // Calculate projection matrix
    glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), camera.aspect, 0.1f, 100.0f);
    renderer_.set_projection_matrix(projection);
    
    renderer_.render();
}