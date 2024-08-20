#pragma once
#include <Quark/Scene/Scene.h>
#include "Editor/Panel/Panel.h"

namespace quark {

class InspectorPanel final: public Panel {
public:
    InspectorPanel();

    void OnImGuiUpdate() override;

    void SetScene(Scene* scene) { m_Scene = scene; }
    void SetSelectedEntity(Entity* entity) { m_SelectedEntity = entity; }

private:
    Entity* m_SelectedEntity;
    Scene* m_Scene;
    bool m_Rename;
    char m_Buf[128];

};

}