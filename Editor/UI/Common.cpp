#include "Editor/UI/Common.h"
#include <imgui.h>
#include <imgui_internal.h>
namespace editor::ui {
bool DrawVec3Control(const char* label, glm::vec3& vector, float reset , float columnWidth )
{
	bool changed = false;

	ImGui::PushID(label);

	ImGui::Columns(2);
	ImGui::SetColumnWidth(0, columnWidth);

	ImGui::Text("%s", label);
	ImGui::NextColumn();

	ImGui::PushMultiItemsWidths(3, ImGui::CalcItemWidth());
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{ 0,0 });

	float lineheight = GImGui->Font->FontSize + GImGui->Style.FramePadding.y * 2.f;
	ImVec2 buttonsize = { lineheight + 3.f,lineheight };

	auto& BoldFont = ImGui::GetIO().Fonts->Fonts[0];

	ImGui::PushStyleColor(ImGuiCol_Button, { 0.8f,0.1f,0.1f,1.f });
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, { 0.7f,0.1f,0.1f,1.f });
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, { 0.6f,0.1f,0.1f,1.f });
	ImGui::PushFont(BoldFont);
	if (ImGui::Button("X", buttonsize))
	{
		vector.x = reset;
		changed = true;
	}
	ImGui::PopStyleColor(3);
	ImGui::PopFont();

	ImGui::SameLine();
	if (ImGui::DragFloat("##X", &vector.x, 0.1f))
		changed = true;
	ImGui::PopItemWidth();
	ImGui::SameLine();

	ImGui::PushStyleColor(ImGuiCol_Button, { 0.1f,0.8f,0.1f,1.f });
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, { 0.1f,0.7f,0.1f,1.f });
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, { 0.1f,0.6f,0.1f,1.f });
	ImGui::PushFont(BoldFont);
	if (ImGui::Button("Y", buttonsize))
	{
		vector.y = reset;
		changed = true;
	}
	ImGui::PopStyleColor(3);
	ImGui::PopFont();

	ImGui::SameLine();
	if (ImGui::DragFloat("##Y", &vector.y, 0.1f))
		changed = true;
	ImGui::PopItemWidth();
	ImGui::SameLine();

	ImGui::PushStyleColor(ImGuiCol_Button, { 0.1f,0.1f,0.8f,1.f });
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, { 0.1f,0.1f,0.7f,1.f });
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, { 0.1f,0.1f,0.6f,1.f });
	ImGui::PushFont(BoldFont);
	if (ImGui::Button("Z", buttonsize))
	{
		vector.z = reset;
		changed = true;
	}
	ImGui::PopStyleColor(3);
	ImGui::PopFont();

	ImGui::SameLine();
	if (ImGui::DragFloat("##Z", &vector.z, 0.1f))
		changed = true;
	ImGui::PopItemWidth();
	ImGui::SameLine();

	ImGui::PopStyleVar();

	ImGui::Columns(1);

	ImGui::PopID();

	return changed;
}

}