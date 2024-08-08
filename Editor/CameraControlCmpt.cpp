#include "Editor/CameraControlCmpt.h"
#include <Quark/Events/EventManager.h>

namespace quark {

EditorCameraControlCmpt::EditorCameraControlCmpt(float moveSpeed, float mouseSensitivity)
    : MoveControlCmpt(moveSpeed, mouseSensitivity), isViewPortTouching_(false)
{
    EventManager::Instance().Subscribe<SceneViewPortTouchedEvent>([&](const SceneViewPortTouchedEvent& e) {
        OnViewPortTouchedEvent(e);
    });
}

void EditorCameraControlCmpt::Update(float deltaTime)
{   
    if (isViewPortTouching_) {
        MoveControlCmpt::Update(deltaTime);
        isViewPortTouching_ = false;
    }

    lastPosition_ = Input::Singleton()->GetMousePosition();

}

void EditorCameraControlCmpt::OnViewPortTouchedEvent(const SceneViewPortTouchedEvent& e)
{
    isViewPortTouching_ = true;
}
}