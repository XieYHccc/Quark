#pragma once
#include <glm/glm.hpp>

#include "GameObject/Components/Component.h"
#include "GameObject/Components/TransformCmpt.h"

class CameraCmpt : public Component {
public:
    float fov;
    float aspect;
    float zNear;
    float zFar;
    float movementSpeed;

public:
    using Component::Component;
    COMPONENT_TYPE("CameraCmpt")

    void Awake() override;
    
    glm::mat4 GetViewMatrix() const ;
    glm::mat4 GetProjectionMatrix() const ;

private:
    TransformCmpt* transform;
};