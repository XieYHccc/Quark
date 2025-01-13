#include "Quark/qkpch.h"
#include "Quark/Core/Window.h"
#include "Quark/Ecs/Entity.h"
#include "Quark/Events/EventManager.h"
#include "Quark/Scene/Components/CameraCmpt.h"

#include <glm/gtx/transform.hpp>


namespace quark {

CameraCmpt::CameraCmpt(float _aspect, float _fov, float _zNear, float _zFar)
    : aspect(_aspect), fov(_fov), zNear(_zNear), zFar(_zFar)
{
    EventManager::Get().Subscribe<WindowResizeEvent>([this](const WindowResizeEvent& e) {
        aspect = static_cast<float>(e.width) / e.height;
    });

}

glm::mat4 CameraCmpt::GetViewMatrix()
{
    // view matrix transform geometry from world space to eye/view space
    // 没有相机这一概念的时候相当于相机位于原点朝向（0, 0, -1), view space 就是 world space.
    auto* transform = GetEntity()->GetComponent<TransformCmpt>();
    return glm::inverse(transform->GetLocalMatrix());
}

glm::mat4 CameraCmpt::GetProjectionMatrix()
{
    return glm::perspective(glm::radians(fov), aspect, zNear, zFar);
}

}