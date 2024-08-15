#pragma once
#include <glm/glm.hpp>
#include <Quark/Scene/Components/MoveControlCmpt.h>
#include <Quark/Events/EventHandler.h>

#include "Editor/UI/SceneViewPort.h"

namespace quark {
class EditorCameraControlCmpt : public quark::MoveControlCmpt {
public:
    QK_COMPONENT_TYPE_DECL(EditorCameraControlCmpt)
    
    EditorCameraControlCmpt(float moveSpeed = 20, float mouseSensitivity = 0.3);
    ~EditorCameraControlCmpt();

    void Update(float deltaTime) override;
    void OnViewPortTouchedEvent(const SceneViewPortTouchedEvent& e);

private:
    bool m_IsViewPortTouching;
    EventCallbackFn<SceneViewPortTouchedEvent> m_ViewPortTouchedEventCallback;
};
}