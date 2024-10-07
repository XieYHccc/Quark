#pragma once
#include <Quark/Scene/Scene.h>

#include "Editor/Panel/Panel.h"

namespace quark {
class SceneHeirarchyPanel final: public Panel {
public:
    SceneHeirarchyPanel();
    SceneHeirarchyPanel(Ref<Scene> scene);

    void OnImGuiUpdate() override;

    void SetScene(Ref<Scene> scene);
    void SetSelectedEntity(Entity* entity) { m_SelectedEntity = entity; }

    Entity* GetSelectedEntity() { return m_SelectedEntity; }

private:
    void DrawEntity(Entity* entity);

    Ref<Scene> m_Scene;

    Entity* m_SelectedEntity;

    std::vector<Entity*> m_GarbageEntities;

};

}