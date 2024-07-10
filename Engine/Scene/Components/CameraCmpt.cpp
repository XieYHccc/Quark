#include "pch.h"
#include <glm/gtx/transform.hpp>
#include "Core/Window.h"
#include "Scene/Components/CameraCmpt.h"

namespace scene {

CameraCmpt::CameraCmpt(Entity* entity, float _aspect, float _fov, float _zNear, float _zFar)
    : Component(entity), aspect(_aspect), fov(_fov), zNear(_zNear), zFar(_zFar)
{
    transform_ = entity->GetComponent<TransformCmpt>();
}

glm::mat4 CameraCmpt::GetViewMatrix() const 
{
    // view matrix transform geometry from world space to eye/view space
    // 没有相机这一概念的时候相当于相机位于原点朝向（0, 0, -1), view space 就是 world space.
    return glm::inverse(transform_->GetTRSMatrix());
}

glm::mat4 CameraCmpt::GetProjectionMatrix() const
{
    return glm::perspective(glm::radians(fov), aspect, zNear, zFar);
}

}