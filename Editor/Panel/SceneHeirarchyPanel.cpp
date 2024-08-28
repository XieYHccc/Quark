#include "Editor/Panel/SceneHeirarchyPanel.h"

#include <Quark/UI/UI.h>
#include <Quark/Scene/Components/CommonCmpts.h>
#include <Quark/Scene/Components/RelationshipCmpt.h>
#include <imgui.h>

namespace quark {

SceneHeirarchyPanel::SceneHeirarchyPanel() : m_Scene(nullptr), m_SelectedEntity(nullptr)
{

}

SceneHeirarchyPanel::SceneHeirarchyPanel(Ref<Scene> scene) : m_Scene(scene), m_SelectedEntity(nullptr)
{

}


void SceneHeirarchyPanel::SetScene(Ref<Scene> scene)
{
    m_Scene = scene;
    m_SelectedEntity = nullptr;
}

void SceneHeirarchyPanel::OnImGuiUpdate()
{
    if (m_Scene == nullptr)
        return;

    if(ImGui::Begin("Scene Heirarchy")) {

        // Copy the vector avoid invalid iterator issue, when adding or removing entities
        std::vector<Entity*> entities = m_Scene->GetAllEntitiesWith<IdCmpt, RelationshipCmpt>();
        for (auto* e : entities)
        {
            if (e->GetComponent<RelationshipCmpt>()->GetParentEntity() == nullptr)
                DrawEntity(e);
        }

        if (ImGui::IsMouseDown(0) && ImGui::IsWindowHovered())
        {
            m_SelectedEntity = nullptr;
        }

        if (m_SelectedEntity == nullptr && ImGui::BeginPopupContextWindow(0, ImGuiPopupFlags_MouseButtonRight))
        {
            if (ImGui::MenuItem("Create Node")) {
                m_Scene->CreateEntity("Not named", nullptr);
            }
            
            ImGui::EndPopup();
        }
    }
    ImGui::End();

    // Delete garbage entities
    for (auto* e : m_GarbageEntities)
    {
        if (e == m_SelectedEntity)
            m_SelectedEntity = nullptr;

        m_Scene->DeleteEntity(e);
    }
    m_GarbageEntities.clear();
}

void SceneHeirarchyPanel::DrawEntity(Entity* entity)
{
    if (entity == nullptr)
        return;
    
    const std::vector<Entity*>& children = entity->GetComponent<RelationshipCmpt>()->GetChildEntities();

	ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_SpanAvailWidth;
    flags |= m_SelectedEntity == entity ? ImGuiTreeNodeFlags_Selected : 0;
    flags |= children.empty() ? ImGuiTreeNodeFlags_Leaf : 0;

    auto* nameCmpt = entity->GetComponent<NameCmpt>();

    bool should_delete = false;
    bool opened = ImGui::TreeNodeEx((void*)(uint64_t)entity, flags, "%s", nameCmpt? nameCmpt->name.c_str() : "");

    if (ImGui::IsItemClicked())
        m_SelectedEntity = entity;

	if (ImGui::BeginPopupContextItem()) 
    {
        if (ImGui::MenuItem("Add child")) {
            m_Scene->CreateEntity("Not named", entity);
        }

        if (ImGui::MenuItem("Remove")) {
            auto* parentEntity = entity->GetComponent<RelationshipCmpt>()->GetParentEntity();
            auto* parentRelationshipCmpt = parentEntity->GetComponent<RelationshipCmpt>();
            parentRelationshipCmpt->RemoveChildEntity(entity);
        }

		if (ImGui::MenuItem("Delete"))
			should_delete = true;

        ImGui::EndPopup();
	}

    if (opened) 
    {
        if (!children.empty()) 
        {
            for (auto* c : children)
                DrawEntity(c); 
        }

        ImGui::TreePop(); 
    }

    if (should_delete) 
    {   
        m_GarbageEntities.push_back(entity);
        return;
    }

}

}