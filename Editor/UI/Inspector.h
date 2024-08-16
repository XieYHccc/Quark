#pragma once
#include <Quark/Scene/Scene.h>
#include "Editor/UI/Common.h"

namespace quark {

class Inspector : public UIWindowBase {
public:
    Inspector();

    virtual void Render() override;

    void SetScene(Scene* scene) { m_Scene = scene; }
    void SetSelectedEntity(Entity* entity) { m_SelectedEntity = entity; }

private:
    Entity* m_SelectedEntity;
    Scene* m_Scene;
    bool m_Rename;
    char m_Buf[128];

};

}