#include "Editor/UI/Inspector.h"
#include <imgui.h>
#include <imgui_internal.h>

#include <Quark/Scene/Components/CameraCmpt.h>
#include <Quark/Scene/Components/MeshCmpt.h>
#include <Quark/Scene/Components/CommonCmpts.h>

namespace quark {

void Inspector::Init()
{
    m_SelectedEntity = nullptr;
    rename_ = false;
    buf_[0] = '\0';
}

template<typename T, typename UIFunction>
static void DrawComponent(const std::string& name, Entity* entity, UIFunction uiFunction)
{
    const ImGuiTreeNodeFlags treeNodeFlags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed |
        ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_FramePadding;

    ImGuiIO& io = ImGui::GetIO();
    if (entity->HasComponent<T>())
    {
        auto& component = *entity->GetComponent<T>();
        ImVec2 contentRegionAvailable = ImGui::GetContentRegionAvail();
        float lineHeight = GImGui->Font->FontSize + GImGui->Style.FramePadding.y * 2.0f;

        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{ 4, 4 });
        ImGui::Separator();
        bool open = ImGui::TreeNodeEx((void*)T::GetStaticComponentType(), treeNodeFlags, "%s", name.c_str());
        ImGui::PopStyleVar();
        ImGui::SameLine(contentRegionAvailable.x - lineHeight * 0.5f);
        if (ImGui::Button("+", ImVec2{ lineHeight, lineHeight }))
        {
            ImGui::OpenPopup("ComponentSettings");
        }

        bool removeComponent = false;
        if (ImGui::BeginPopup("ComponentSettings"))
        {
            if (ImGui::MenuItem("Remove component"))
                removeComponent = true;

            ImGui::EndPopup();
        }

        if (open)
        {
            uiFunction(component);
            ImGui::TreePop();
        }

        if (removeComponent)
            entity->RemoveComponent<T>();
    }
}
void Inspector::Render()
{
    if (ImGui::Begin("Inspector"))
    {
        if (m_SelectedEntity == nullptr) {
            ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "No selected node");
            ImGui::End();
            return;
        }
        
        // Name component
        {   
            auto* nameCmpt = m_SelectedEntity->GetComponent<NameCmpt>();
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
            auto* transformCmpt = m_SelectedEntity->GetComponent<TransformCmpt>();
            if (transformCmpt) {
                glm::vec3 position = transformCmpt->GetPosition();
                glm::vec3 scale = transformCmpt->GetScale();
                glm::vec3 eulerAngles = glm::degrees(glm::eulerAngles(transformCmpt->GetQuat()));

                const ImGuiTreeNodeFlags treeNodeFlags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_FramePadding |
                    ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_SpanAvailWidth;
                
                if (ImGui::TreeNodeEx((void*)TransformCmpt::GetStaticComponentType(), treeNodeFlags, "Transform"))
                {
                    if (DrawVec3Control("Position", position))
                        transformCmpt->SetPosition(position);

                    if (DrawVec3Control("Rotation", eulerAngles))
                    {
                        if (eulerAngles.x <= 90 && eulerAngles.y <= 90 && eulerAngles.z <= 90
                            && eulerAngles.x >= -90 && eulerAngles.y >= -90 && eulerAngles.z >= -90) {
                            transformCmpt->SetEuler(glm::radians(eulerAngles));
                        }
                        else
                            CORE_LOGW("Rotation values must be less than 90 degrees");
                    }

                    if (DrawVec3Control("Scale", scale))
                        transformCmpt->SetScale(scale);

                    ImGui::TreePop();
                }

            }
        }

        // Mesh component
        DrawComponent<MeshCmpt>("Mesh", m_SelectedEntity, [&](auto& component) {
            Mesh& mesh = *component.sharedMesh;

            ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x - 125.f);
			ImGui::LabelText("##Name", "%s", "TODO");
			ImGui::PopItemWidth();

			ImGui::SameLine();
			if (ImGui::Button("Set", ImVec2(80.f, 25.f)))
				ImGui::OpenPopup("Set Mesh");
        });


        // Camera component
        DrawComponent<CameraCmpt>("Camera", m_SelectedEntity, [&](auto& component) {
            float perspectiveVerticalFov = component.fov;
            if (ImGui::DragFloat("Vertical FOV", &perspectiveVerticalFov))
                component.fov = perspectiveVerticalFov;

            float perspectiveNear = component.zNear;
            if (ImGui::DragFloat("Near", &perspectiveNear))
                component.zNear = perspectiveNear;

            float perspectiveFar = component.zFar;
            if (ImGui::DragFloat("Far", &perspectiveFar))
                component.zFar = perspectiveFar;
        });
    }
    ImGui::End();
}

}