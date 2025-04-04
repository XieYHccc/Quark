#pragma once
#include "Quark/Core/Input.h"
#include "Quark/Core/TimeStep.h"
#include "Quark/Scene/Components/TransformCmpt.h"

namespace quark {

class MoveControlCmpt : public Component {
public:
    QK_COMPONENT_TYPE_DECL(MoveControlCmpt)

    MoveControlCmpt(float moveSpeed = 20, float mouseSensitivity = 0.3);

    void Update(TimeStep deltaTime);
    void SetMoveSpeed(float moveSpeed) { m_MoveSpeed = moveSpeed; }
    void SetMouseSensitivity(float mouseSensitivity) { m_MouseSensitivity = mouseSensitivity; }

    void ProcessKeyInput(TimeStep deltaTime);
    void ProcessMouseMove(float xoffset, float yoffset);

    float m_Yaw ;
    float m_Pitch;
    float m_MoveSpeed;
    float m_MouseSensitivity;
    MousePosition m_LastPosition;
    bool m_IsFirstMouse;
};

}