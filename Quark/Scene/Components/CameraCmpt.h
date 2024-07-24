#pragma once
#include <glm/glm.hpp>
#include "Events/ApplicationEvent.h"
#include "Scene/Components/TransformCmpt.h"

namespace scene {

class CameraCmpt : public Component {
public:
    float fov;
    float aspect;
    float zNear;
    float zFar;

public:
    CameraCmpt(Entity* entity, float aspect, float fov = 60.f, float zNear = 0.1f, float zFar = 100.f);
    QK_COMPONENT_TYPE_DECL(Camera)
    
    glm::mat4 GetViewMatrix() const ;
    glm::mat4 GetProjectionMatrix() const ;

    void OnWindowResize(const WindowResizeEvent& e);
private:
    TransformCmpt* transform_;
};

}