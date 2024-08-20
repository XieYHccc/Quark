#pragma once
#include <Quark/Scene/Scene.h>

#include "Editor/Panel/Panel.h"

namespace quark {
class SceneHeirarchyPanel final: public Panel {
public:
    SceneHeirarchyPanel();
    SceneHeirarchyPanel(Scene* scene);

    void OnImGuiUpdate() override;

    void SetScene(Scene* scene);

    Entity* GetSelectedEntity() { return m_SelectedEntity; }

private:
    void DrawEntity(Entity* entity);

    Scene* m_Scene;
    Entity* m_SelectedEntity;
    std::vector<Entity*> m_GarbageEntities;

};

}