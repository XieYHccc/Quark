#include "Editor/CameraControlCmpt.h"
#include <Quark/Events/EventManager.h>

namespace editor::component {

EditorCameraControlCmpt::EditorCameraControlCmpt(scene::Entity* entity, float moveSpeed, float mouseSensitivity)
    : scene::MoveControlCmpt(entity, moveSpeed, mouseSensitivity), isViewPortTouching_(false)
{
    EventManager::Instance().Subscribe<ui::SceneViewPortTouchedEvent>([&](const ui::SceneViewPortTouchedEvent& e) {
        OnViewPortTouchedEvent(e);
    });
}

void EditorCameraControlCmpt::Update(float deltaTime)
{   
    if (isViewPortTouching_) {
        scene::MoveControlCmpt::Update(deltaTime);
        isViewPortTouching_ = false;
    }

    lastPosition_ = Input::Singleton()->GetMousePosition();

}

void EditorCameraControlCmpt::OnViewPortTouchedEvent(const ui::SceneViewPortTouchedEvent& e)
{
    isViewPortTouching_ = true;
}

}