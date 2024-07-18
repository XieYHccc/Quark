#pragma once
#include "Core/Input.h"
#include "Scene/Ecs.h"
#include "Scene/Components/TransformCmpt.h"

namespace scene {
class MoveControlCmpt : public Component {
public:
    QK_COMPONENT_TYPE_DECL(MoveControlCmpt)

    MoveControlCmpt(Entity* entity, float moveSpeed = 20, float mouseSensitivity = 0.3);

    void Update(float deltaTime);
    void SetMoveSpeed(float moveSpeed) { moveSpeed_ = moveSpeed; }
    void SetMouseSensitivity(float mouseSensitivity) { mouseSensitivity_ = mouseSensitivity; }

private:
    void ProcessKeyInput(float deltaTime);
    void ProcessMouseMove(float xoffset, float yoffset);

    float yaw_ ;
    float pitch_;
    float moveSpeed_;
    float mouseSensitivity_;
    TransformCmpt* transform_;
    MousePosition lastPosition_;
    bool isFirstMouse_;
};
}