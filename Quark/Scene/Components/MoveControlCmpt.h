#pragma once
#include "Quark/Core/Input.h"
#include "Quark/Ecs/Entity.h"
#include "Quark/Scene/Components/TransformCmpt.h"

namespace quark {

class MoveControlCmpt : public Component {
public:
    QK_COMPONENT_TYPE_DECL(MoveControlCmpt)

    MoveControlCmpt(float moveSpeed = 20, float mouseSensitivity = 0.3);

    virtual void Update(float deltaTime);
    void SetMoveSpeed(float moveSpeed) { moveSpeed_ = moveSpeed; }
    void SetMouseSensitivity(float mouseSensitivity) { mouseSensitivity_ = mouseSensitivity; }

protected:
    void ProcessKeyInput(float deltaTime);
    void ProcessMouseMove(float xoffset, float yoffset);

    float yaw_ ;
    float pitch_;
    float moveSpeed_;
    float mouseSensitivity_;
    MousePosition lastPosition_;
    bool isFirstMouse_;
};

}