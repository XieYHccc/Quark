#include "Editor/UI/Inspector.h"
#include <imgui.h>

namespace editor::ui {

void Inspector::Render()
{
    if (ImGui::Begin("Inspector"))
    {
        if (selectedNode_ == nullptr) {
            ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "No selected node");
            ImGui::End();
            return;
        }

        auto* entity = selectedNode_->GetEntity();
        
        // Name component
        auto* nameCmpt = entity->GetComponent<scene::NameCmpt>();
        char buffer[256];
        strncpy(buffer, nameCmpt->name.c_str(), sizeof(buffer));

        if (ImGui::InputText("##Name", buffer, sizeof(buffer)))
            nameCmpt->name = buffer;
        
        // Button : add component
		ImGui::SameLine();
		ImGui::PushItemWidth(-1);
        if (ImGui::Button("Add Component"))
			ImGui::OpenPopup("AddComponent");
        
        // Transform component
        auto* transformCmpt = entity->GetComponent<scene::TransformCmpt>();
        if (transformCmpt) {
            const ImGuiTreeNodeFlags treeNodeFlags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_AllowItemOverlap
			| ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_FramePadding;

            if (ImGui::TreeNodeEx((void*)scene::TransformCmpt::GetStaticComponentType(), treeNodeFlags, "Transform")) {
                if(ImGui::InputFloat3("position", (float*)&transformCmpt->GetPosition()[0])){
                    
                }
                
                ImGui::TreePop();
            }

        }


    }
    ImGui::End();
}

}