#include "Editor/UI/SceneHeirarchy.h"
#include <Quark/UI/UI.h>
#include "Quark/Scene/Components/CommonCmpts.h"
#include <imgui.h>

namespace quark {

SceneHeirarchy::SceneHeirarchy() : scene_(nullptr), selectedObject_(nullptr)
{

}

SceneHeirarchy::SceneHeirarchy(Scene* scene) : scene_(scene), selectedObject_(nullptr)
{

}

void SceneHeirarchy::Init()
{
    scene_ = nullptr;
    selectedObject_ = nullptr;
}

void SceneHeirarchy::SetScene(Scene *scene)
{
    scene_ = scene;
    selectedObject_ = scene->GetRootGameObject();
}

void SceneHeirarchy::Render()
{
    if (scene_ == nullptr)
        return;

    if(ImGui::Begin("Scene Heirarchy")) {
        DrawNode(scene_->GetRootGameObject());

        if (ImGui::IsMouseDown(0) && ImGui::IsWindowHovered())
        {
            selectedObject_ = nullptr;
        }

        if (selectedObject_ == nullptr && ImGui::BeginPopupContextWindow(0, ImGuiPopupFlags_MouseButtonRight))
        {
            if (ImGui::MenuItem("Create Node")) {
                scene_->CreateGameObject("Not named", scene_->GetRootGameObject());
            }
            
            ImGui::EndPopup();
        }
    }

    ImGui::End();
}

void SceneHeirarchy::DrawNode(GameObject* obj)
{
    if (obj == nullptr)
        return;
    
    const std::vector<GameObject*>& children = obj->GetChildren();

	ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_SpanAvailWidth;
    flags |= selectedObject_ == obj ? ImGuiTreeNodeFlags_Selected : 0;
    flags |= children.empty() ? ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen : 0;

    auto* nameCmpt = obj->GetEntity()->GetComponent<NameCmpt>();

    bool should_delete = false;
    bool opened = ImGui::TreeNodeEx((void*)(uint64_t)obj, flags, "%s", nameCmpt? nameCmpt->name.c_str() : "");

    if (ImGui::IsItemClicked())
        selectedObject_ = obj;

	if (ImGui::BeginPopupContextItem()) {
        if (ImGui::MenuItem("Add child")) {
            scene_->CreateGameObject("Not named", obj);
        }

        if (ImGui::MenuItem("Remove")) {
            obj->GetParent()->RemoveChild(obj);
        }

		if (ImGui::MenuItem("Delete"))
			should_delete = true;

        ImGui::EndPopup();
	}

    if (opened && !children.empty()) {
        for (auto* child : obj->GetChildren())
            DrawNode(child);
    
        ImGui::TreePop();

    }

    if (should_delete) {
        if (selectedObject_ == obj)
            selectedObject_ = nullptr;
        
        scene_->DeleteGameObject(obj);

        return;
    }

}

}