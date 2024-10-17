#pragma once
#include "Quark/Events/ApplicationEvent.h"
#include "Quark/Scene/Components/TransformCmpt.h"

#include <glm/glm.hpp>

namespace quark {

struct CameraCmpt : public Component {
public:
    float fov;
    float aspect;
    float zNear;
    float zFar;

    CameraCmpt(float aspect = 1.f, float fov = 60.f, float zNear = 0.1f, float zFar = 100.f);
    QK_COMPONENT_TYPE_DECL(Camera)
    
    glm::mat4 GetViewMatrix();
    glm::mat4 GetProjectionMatrix();

    void OnWindowResize(const WindowResizeEvent& e);
};

}