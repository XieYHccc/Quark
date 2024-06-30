#include "pch.h"
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/transform.hpp>

#include "Core/Window.h"
#include "Scene/Components/CameraCmpt.h"

namespace scene {

void CameraCmpt::Awake()
{
    transform_ = GetEntity()->GetComponent<TransformCmpt>();

    int width = Window::Instance()->GetWidth();
    int height = Window::Instance()->GetHeight();

    // default camera properties
    this->fov = 45.f;
    this->zNear = 1000.f;
    this->zFar = 0.1f;
    this->aspect = (float)width / height;
    this->movementSpeed = 20.f;


}
glm::mat4 CameraCmpt::GetViewMatrix() const 
{
    // view matrix transform geometry from world space to eye/view space
    // 没有相机这一概念的时候相当于相机位于原点朝向（0, 0, -1), view space 就是 world space.
    return glm::inverse(transform_->GetTRSMatrix());
}

glm::mat4 CameraCmpt::GetProjectionMatrix() const
{
    return glm::perspective(glm::radians(70.f), aspect, zNear, zFar);
}

}