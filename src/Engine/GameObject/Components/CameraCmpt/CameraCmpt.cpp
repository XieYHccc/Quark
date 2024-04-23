#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/transform.hpp>

#include "GameObject/Components/CameraCmpt/CameraCmpt.h"
#include "GameObject/GameObject.h"
#include "Application/Application.h"

void CameraCmpt::Awake()
{
    transform = GetOwner()->transformCmpt;

    int width = Application::Instance().GetWindow().GetWidth();
    int height = Application::Instance().GetWindow().GetHeight();

    // default camera properties
    this->fov = 45.f;
    this->zNear = 1000.f;
    this->zFar = 0.1f;
    this->aspect = (float)width / height;
    this->movementSpeed = 20.f;


}
glm::mat4 CameraCmpt::GetViewMatrix() const 
{
    // there is no scale for a camera object
    XE_DYNAMIC_ASSERT(transform->GetScale() == glm::vec3(1.f, 1.f, 1.f))

    // view matrix transform geometry from world space to eye/view space
    // 没有相机这一概念的时候相当于相机位于原点朝向（0, 0, -1), view space 就是 world space.
    return glm::inverse(transform->GetTRSMatrix());
}

glm::mat4 CameraCmpt::GetProjectionMatrix() const
{
    return glm::perspective(glm::radians(70.f), aspect, zNear, zFar);
}