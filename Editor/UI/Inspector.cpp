#include "Editor/UI/Inspector.h"
#include <imgui.h>

namespace editor::ui {

void Inspector::Init()
{
    selectedNode_ = nullptr;
    rename_ = false;
    buf_[0] = '\0';
}

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
        {   
            auto* nameCmpt = entity->GetComponent<scene::NameCmpt>();
            char buffer[256];
            strncpy(buffer, nameCmpt->name.c_str(), sizeof(buffer));

            ImGui::AlignTextToFramePadding();
            ImGui::Text("Node name:");
            ImGui::SameLine();
            if (ImGui::InputText("##Name", buffer, sizeof(buffer)))
                nameCmpt->name = buffer;
        }
        
        // Transform component
        {
            auto* transformCmpt = entity->GetComponent<scene::TransformCmpt>();
            if (transformCmpt) {
                glm::vec3 position = transformCmpt->GetPosition();
                glm::vec3 scale = transformCmpt->GetScale();
                glm::vec3 eulerAngles = glm::degrees(glm::eulerAngles(transformCmpt->GetQuat()));

                const ImGuiTreeNodeFlags treeNodeFlags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_SpanAvailWidth;
                
                if (ImGui::TreeNodeEx((void*)scene::TransformCmpt::GetStaticComponentType(), treeNodeFlags, "Transform")) {

                    ImGui::AlignTextToFramePadding();
                    ImGui::Text("Position:");
                    ImGui::SameLine();
                    if(ImGui::InputFloat3("##Position", (float*)&position)){
                        transformCmpt->SetPosition(position);
                    }

                    ImGui::AlignTextToFramePadding();
                    ImGui::Text("Rotation:");
                    ImGui::SameLine();
                    if(ImGui::InputFloat3("##Rotation", (float*)&eulerAngles)){
                        if (eulerAngles.x <= 90 && eulerAngles.y <= 90 && eulerAngles.z <= 90
                            && eulerAngles.x >= -90 && eulerAngles.y >= -90 && eulerAngles.z >= -90) {
                            transformCmpt->SetEuler(glm::radians(eulerAngles));
                        }
                        else {
                            CORE_LOGW("Rotation values must be less than 90 degrees");
                        }
                    }

                    ImGui::AlignTextToFramePadding();
                    ImGui::Text("Scale:");
                    ImGui::SameLine();
                    if(ImGui::InputFloat3("##Scale", (float*)&scale)){
                        transformCmpt->SetScale(scale);
                    }
                    ImGui::TreePop();
                }

            }
        }


    }
    ImGui::End();
}

}