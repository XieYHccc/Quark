#include "Quark/qkpch.h"
#include "Quark/Ecs/Entity.h"
#include "Quark/Scene/Components/MoveControlCmpt.h"

#include <glm/gtx/quaternion.hpp>

namespace quark {

MoveControlCmpt::MoveControlCmpt(float moveSpeed, float mouseSensitivity)
    : m_MoveSpeed(moveSpeed), m_MouseSensitivity(mouseSensitivity)
{
    m_Yaw = 0;
    m_Pitch = 0;
    m_IsFirstMouse = true;
    m_LastPosition = {0, 0};
}

void MoveControlCmpt::Update(TimeStep deltaTime)
{
    MousePosition pos = Input::Get()->GetMousePosition();
    if (m_IsFirstMouse) 
    {
        m_LastPosition = pos;
        m_IsFirstMouse = false;
    }

    // Process mouse movement
    float xoffset = pos.x_pos - m_LastPosition.x_pos;
    float yoffset = pos.y_pos - m_LastPosition.y_pos;
    ProcessMouseMove(xoffset, yoffset);
    m_LastPosition = pos;

    // Process key input
    ProcessKeyInput(deltaTime);
}

void MoveControlCmpt::ProcessMouseMove(float xoffset, float yoffset)
{
    auto* transform = GetEntity()->GetComponent<TransformCmpt>();
    m_Pitch -= (glm::radians(yoffset) * m_MouseSensitivity);
    m_Yaw -= (glm::radians(xoffset) * m_MouseSensitivity);

    // make sure that when pitch is out of bounds, screen doesn't get flipped
    m_Pitch = std::clamp(m_Pitch, -1.5f, 1.5f);

    transform->SetLocalRotate(glm::vec3(m_Pitch, m_Yaw, 0));
}

void MoveControlCmpt::ProcessKeyInput(TimeStep deltaTime)
{   
    auto* transform = GetEntity()->GetComponent<TransformCmpt>();

    glm::vec3 move {0.f};
    if (Input::Get()->IsKeyPressed(Key::W, true))
        move.z = -1;
    if (Input::Get()->IsKeyPressed(Key::S, true))
        move.z = 1;
    if (Input::Get()->IsKeyPressed(Key::A, true))
        move.x = -1;
    if (Input::Get()->IsKeyPressed(Key::D, true))
        move.x = 1;
    move = move * m_MoveSpeed * deltaTime.GetSeconds();
    move = glm::rotate(transform->GetLocalRotate(), move);
    transform->Translate(move);
    // transform->SetLocalPosition(transform->GetPosition() + glm::rotate(transform->GetQuat(), move));
}

}