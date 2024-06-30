#pragma once
#include <glm/glm.hpp>

#include "Scene/Ecs.h"
#include "Scene/Components/TransformCmpt.h"

namespace scene {

class CameraCmpt : public Component {
public:
    float fov;
    float aspect;
    float zNear;
    float zFar;
    float movementSpeed;

public:
    using Component::Component;
    QK_COMPONENT_TYPE_DECL(CameraCmpt)

    void Awake() override;
    
    glm::mat4 GetViewMatrix() const ;
    glm::mat4 GetProjectionMatrix() const ;

private:
    TransformCmpt* transform_;
};

}