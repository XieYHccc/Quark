#include "Editor/CameraControlCmpt.h"

namespace editor::component {

void EditorCameraControlCmpt::Update(float deltaTime)
{   
    // 1. Process mouse movement
    MousePosition pos = Input::Singleton()->GetMousePosition();
    if (isFirstMouse_) {
        lastPosition_ = pos;
        isFirstMouse_ = false;
    }

    if (Input::Singleton()->IsMousePressed(MOUSE_CODE_BUTTON0, true)) {

        float xoffset = pos.x_pos - lastPosition_.x_pos;
        float yoffset = pos.y_pos - lastPosition_.y_pos;
        ProcessMouseMove(xoffset, yoffset);
    }

    lastPosition_ = pos;

    // 2. Process key inputs
    ProcessKeyInput(deltaTime);

}

}