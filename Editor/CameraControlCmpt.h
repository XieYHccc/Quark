#pragma once
#include <glm/glm.hpp>
#include <Scene/Components/MoveControlCmpt.h>
#include <Quark/Events/MouseEvent.h>
#include <Quark/Events/KeyEvent.h>
#include "Editor/UI/SceneViewPort.h"
namespace editor::component {

class EditorCameraControlCmpt : public scene::MoveControlCmpt {
public:
    QK_COMPONENT_TYPE_DECL(EditorCameraControlCmpt)
    
    EditorCameraControlCmpt(scene::Entity* entity, float moveSpeed = 20, float mouseSensitivity = 0.3);
    ~EditorCameraControlCmpt() = default;

    void Update(float deltaTime) override;
    void OnViewPortTouchedEvent(const ui::SceneViewPortTouchedEvent& e);

private:
    bool isViewPortTouching_;
};
}