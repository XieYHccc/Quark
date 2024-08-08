#pragma once
#include <glm/glm.hpp>
#include <Quark/Scene/Components/MoveControlCmpt.h>
#include <Quark/Events/MouseEvent.h>
#include <Quark/Events/KeyEvent.h>
#include "Editor/UI/SceneViewPort.h"

namespace quark {
class EditorCameraControlCmpt : public quark::MoveControlCmpt {
public:
    QK_COMPONENT_TYPE_DECL(EditorCameraControlCmpt)
    
    EditorCameraControlCmpt(float moveSpeed = 20, float mouseSensitivity = 0.3);
    ~EditorCameraControlCmpt() = default;

    void Update(float deltaTime) override;
    void OnViewPortTouchedEvent(const SceneViewPortTouchedEvent& e);

private:
    bool isViewPortTouching_;
};
}