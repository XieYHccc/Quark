#include "qkpch.h"
#include "Scene/Components/MoveControlCmpt.h"
#include <glm/gtx/quaternion.hpp>
namespace scene {
MoveControlCmpt::MoveControlCmpt(Entity* entity, float moveSpeed, float mouseSensitivity)
    : Component(entity), moveSpeed_(moveSpeed), mouseSensitivity_(mouseSensitivity)
{
    transform_ = entity_->GetComponent<TransformCmpt>();
    yaw_ = 0;
    pitch_ = 0;
    isFirstMouse_ = true;
    lastPosition_ = {0, 0};
}

void MoveControlCmpt::Update(float deltaTime)
{
    MousePosition pos = Input::Singleton()->GetMousePosition();
    if (isFirstMouse_) {
        lastPosition_ = pos;
        isFirstMouse_ = false;
    }

    // Process mouse movement
    float xoffset = pos.x_pos - lastPosition_.x_pos;
    float yoffset = pos.y_pos - lastPosition_.y_pos;
    ProcessMouseMove(xoffset, yoffset);
    lastPosition_ = pos;

    // Process key input
    ProcessKeyInput(deltaTime);
}

void MoveControlCmpt::ProcessMouseMove(float xoffset, float yoffset)
{

    pitch_ -= (glm::radians(yoffset) * mouseSensitivity_);
    yaw_ -= (glm::radians(xoffset) * mouseSensitivity_);

    // make sure that when pitch is out of bounds, screen doesn't get flipped
    pitch_ = std::clamp(pitch_, -1.5f, 1.5f);

    transform_->SetEuler(glm::vec3(pitch_, yaw_, 0));
}

void MoveControlCmpt::ProcessKeyInput(float deltaTime)
{   
    // Convert deltaTime from ms to s
    deltaTime = deltaTime / 1000;

    glm::vec3 move {0.f};
    if (Input::Singleton()->IsKeyPressed(KEY_CODE_W, true))
        move.z = -1;
    if (Input::Singleton()->IsKeyPressed(KEY_CODE_S, true))
        move.z = 1;
    if (Input::Singleton()->IsKeyPressed(KEY_CODE_A, true))
        move.x = -1;
    if (Input::Singleton()->IsKeyPressed(KEY_CODE_D, true))
        move.x = 1;
    move = move * moveSpeed_ * deltaTime;

    glm::mat4 rotationMatrix = glm::toMat4(transform_->GetQuat());
    transform_->SetPosition(transform_->GetPosition() + glm::vec3(rotationMatrix * glm::vec4(move, 0.f)));
}
}