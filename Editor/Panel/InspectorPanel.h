#pragma once
#include <Quark/Asset/MaterialAsset.h>
#include <Quark/Scene/Scene.h>

#include "Editor/Panel/Panel.h"

namespace quark {

enum class InspectorViewType {
    ENTITY,
    MATERIAL
};

class InspectorPanel final: public Panel {
public:
    InspectorPanel();

    void OnImGuiUpdate() override;
    void SetScene(Ref<Scene> scene) { m_Scene = scene; }
    void SetSelectedEntity(Entity* entity) { m_SelectedEntity = entity; }
    void SetInspectorViewType(InspectorViewType type) { m_InspectorViewType = type; }

private:
    void UpdateMaterialView();

    Entity* m_SelectedEntity;
    Ref<Scene> m_Scene;
    Ref<MaterialAsset> m_SelectedMaterial;

    InspectorViewType m_InspectorViewType = InspectorViewType::ENTITY;

    bool m_Rename;
    char m_Buf[128];

};

}