#pragma once
#include <Quark/Scene/Scene.h>
#include "Editor/UI/Common.h"

namespace quark {
class SceneHeirarchy : public UIWindowBase {
public:
    SceneHeirarchy();
    SceneHeirarchy(Scene* scene);

    virtual void Render() override;

    void SetScene(Scene* scene);

    Entity* GetSelectedEntity() { return m_SelectedEntity; }

private:
    void DrawEntity(Entity* entity);
    void DrawSceneSettings();

    Scene* m_Scene;
    Entity* m_SelectedEntity;
    std::vector<Entity*> m_GarbageEntities;

};

}