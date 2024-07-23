#include "Editor/CameraControlCmpt.h"
#include <Quark/Events/EventManager.h>

namespace editor::component {

EditorCameraControlCmpt::EditorCameraControlCmpt(scene::Entity* entity, float moveSpeed, float mouseSensitivity)
    : scene::MoveControlCmpt(entity, moveSpeed, mouseSensitivity), isViewPortTouching_(false)
{
    // Register event callback
    // EventManager::Instance().Subscribe<MouseMovedEvent>([&](const MouseMovedEvent& e) {
    //     OnMouseMoveEvent(e);
    // });

    EventManager::Instance().Subscribe<ui::SceneViewPortTouchedEvent>([&](const ui::SceneViewPortTouchedEvent& e) {
        OnViewPortTouchedEvent(e);
    });
}

void EditorCameraControlCmpt::OnMouseMoveEvent(const MouseMovedEvent& e)
{
    if (isFirstMouse_) {
        lastPosition_ = {e.mouseX, e.mouseY};
        isFirstMouse_ = false;
    }

    if (Input::Singleton()->IsMousePressed(MOUSE_CODE_BUTTON0, true)) {
        float xoffset = e.mouseX - lastPosition_.x_pos;
        float yoffset = e.mouseY - lastPosition_.y_pos;
        ProcessMouseMove(xoffset, yoffset);
    }

    lastPosition_ = {e.mouseX, e.mouseY};
}


void EditorCameraControlCmpt::Update(float deltaTime)
{   
    // // Only need to process key input in fixed update
    // ProcessKeyInput(deltaTime);

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