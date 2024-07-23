#include "Editor/UI/SceneHeirarchy.h"
#include <Quark/UI/UI.h>
#include "Quark/Scene/Components/NameCmpt.h"
#include <imgui.h>

namespace editor::ui {

SceneHeirarchy::SceneHeirarchy() : scene_(nullptr), selectedNode_(nullptr)
{

}

SceneHeirarchy::SceneHeirarchy(scene::Scene* scene) : scene_(scene), selectedNode_(nullptr)
{

}

void SceneHeirarchy::Init()
{
    scene_ = nullptr;
    selectedNode_ = nullptr;
}

void SceneHeirarchy::SetScene(scene::Scene *scene)
{
    scene_ = scene;
    selectedNode_ = scene->GetRootNode();
}

void SceneHeirarchy::Render()
{
    if (scene_ == nullptr)
        return;

    if(ImGui::Begin("Scene Heirarchy")) {
        DrawNode(scene_->GetRootNode());

        if (ImGui::IsMouseDown(0) && ImGui::IsWindowHovered())
        {
            selectedNode_ = nullptr;
        }

        if (selectedNode_ == nullptr && ImGui::BeginPopupContextWindow(0, ImGuiPopupFlags_MouseButtonRight))
        {
            if (ImGui::MenuItem("Create Node")) {
                scene_->CreateNode("Not named", scene_->GetRootNode());
            }
            
            ImGui::EndPopup();
        }
    }

    ImGui::End();
}

void SceneHeirarchy::DrawNode(scene::Node* node)
{
    if (node == nullptr)
        return;
    
    const std::vector<scene::Node*> children = node->GetChildren();

	ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_SpanAvailWidth;
    flags |= selectedNode_ == node ? ImGuiTreeNodeFlags_Selected : 0;
    flags |= children.empty() ? ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen : 0;

    auto* nameCmpt = node->GetEntity()->GetComponent<scene::NameCmpt>();

    bool should_delete = false;
    bool opened = ImGui::TreeNodeEx((void*)(uint64_t)node, flags, "%s", nameCmpt? nameCmpt->name.c_str() : "");

    if (ImGui::IsItemClicked())
        selectedNode_ = node;

	if (ImGui::BeginPopupContextItem()) {
        if (ImGui::MenuItem("Add child")) {
            scene_->CreateNode("Not named", node);
        }

        if (ImGui::MenuItem("Remove")) {
            node->GetParent()->RemoveChild(node);
        }

		if (ImGui::MenuItem("Delete"))
			should_delete = true;

        ImGui::EndPopup();
	}

    if (opened && !children.empty()) {
        for (auto* child : node->GetChildren())
            DrawNode(child);
    
        ImGui::TreePop();

    }

    if (should_delete) {
        if (selectedNode_ == node)
            selectedNode_ = nullptr;
        
        scene_->DeleteNode(node);

        return;
    }

}

}