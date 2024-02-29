#include "./mesh_displayer.h"

#include <iostream>

#include <rttr/registration.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/transform2.hpp>
#include <glm/gtx/euler_angles.hpp>

#include "../../core/object.h"
#include "../../core/transform.h"
#include "../camera.h"
#include "./mesh_filter.h"

using namespace rttr;
RTTR_REGISTRATION
{
    registration::class_<MeshDisplayer>("MeshDisplayer")
            .constructor<>()(rttr::policy::ctor::as_raw_ptr);
}

MeshDisplayer::MeshDisplayer() : renderer_() {}

void MeshDisplayer::render() {
    if (!renderer_.valid_vao()) {
        auto mesh_filter = dynamic_cast<MeshFilter*>(get_object()->get_component("MeshFilter"));
        if (!mesh_filter) {
            std::cerr << "MeshDisplayer::render() : object doesn't have a mesh.";
            return;
        }
        renderer_.setup_vao(mesh_filter->mesh());

        // calculate AABB
        for (auto& v : mesh_filter->mesh()->vertices)
            aabb_ += v.position;
    }

    // Calculate model matrix
    auto transform = dynamic_cast<Transform*>(get_object()->get_component("Transform"));
    if (!transform) { return; }
    glm::mat4 trans = glm::translate(transform->get_position());
    auto quat = transform->get_rotation();
    glm::mat4 rotation = glm::mat4_cast(quat);
    glm::mat4 scale = glm::scale(transform->get_scale());
    glm::mat4 move_to_center = glm::translate(-aabb_.center());
    glm::mat4 model = trans * scale * rotation * move_to_center;
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