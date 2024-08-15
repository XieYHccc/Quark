#include "Editor/CameraControlCmpt.h"
#include <Quark/Events/EventManager.h>

namespace quark {

EditorCameraControlCmpt::EditorCameraControlCmpt(float moveSpeed, float mouseSensitivity)
    : MoveControlCmpt(moveSpeed, mouseSensitivity), m_IsViewPortTouching(false)
{
    m_ViewPortTouchedEventCallback = [&](const SceneViewPortTouchedEvent& e) {
        OnViewPortTouchedEvent(e);
    };

    EventManager::Instance().Subscribe<SceneViewPortTouchedEvent>(m_ViewPortTouchedEventCallback);
}

EditorCameraControlCmpt::~EditorCameraControlCmpt()
{
    EventManager::Instance().Unsubscribe<SceneViewPortTouchedEvent>(m_ViewPortTouchedEventCallback);

}

void EditorCameraControlCmpt::Update(float deltaTime)
{   
    if (m_IsViewPortTouching) {
        MoveControlCmpt::Update(deltaTime);
        m_IsViewPortTouching = false;
    }

    m_LastPosition = Input::Get()->GetMousePosition();

}

void EditorCameraControlCmpt::OnViewPortTouchedEvent(const SceneViewPortTouchedEvent& e)
{
    m_IsViewPortTouching = true;
}

}