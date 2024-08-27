#include "Editor/Panel/ContentBrowserPanel.h"

#include <Quark/Core/FileSystem.h>
#include <Quark/Asset/TextureLoader.h>
#include <Quark/UI/UI.h>

namespace quark {

static std::filesystem::path s_AssetDirectory = "Assets";

ContentBrowserPanel::ContentBrowserPanel()
	:m_CurrentDirectory(s_AssetDirectory)
{
	TextureLoader loader;
	m_FileIcon = loader.LoadStb("Assets/Icons/ContentBrowser/FileIcon.png");
	m_FolderIcon = loader.LoadStb("Assets/Icons/ContentBrowser/DirectoryIcon.png");
	m_FileIconId = UI::Get()->CreateTextureId(m_FileIcon);
	m_FolderIconId = UI::Get()->CreateTextureId(m_FolderIcon);
}

void ContentBrowserPanel::OnImGuiUpdate()
{
	ImGui::Begin("Content Browser");

	if (m_CurrentDirectory != s_AssetDirectory)
	{
		if (ImGui::Button("<-"))
		{
			m_CurrentDirectory = m_CurrentDirectory.parent_path();
		}
	}


	static float padding = 16.0f;
	static float thumbnailSize = 128.0f;
	float cellSize = thumbnailSize + padding;

	float panelWidth = ImGui::GetContentRegionAvail().x;
	int columnCount = (int)(panelWidth / cellSize);
	if (columnCount < 1)
		columnCount = 1;

	ImGui::Columns(columnCount, 0, false);

	for (auto& directoryEntry : std::filesystem::directory_iterator(m_CurrentDirectory))
	{
		const auto& path = directoryEntry.path();
		std::string filenameString = path.filename().string();
		ImTextureID textureId = directoryEntry.is_directory() ? m_FolderIconId : m_FileIconId;

		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
		ImGui::ImageButton(filenameString.c_str(), textureId, { thumbnailSize, thumbnailSize }, { 0, 1 }, { 1, 0 });
		ImGui::PopStyleColor();

		if (ImGui::BeginDragDropSource())
		{
			std::string pathString = path.string();
			ImGui::TextUnformatted(filenameString.c_str());
			ImGui::SetDragDropPayload("CONTENT_BROWSER_ITEM", pathString.c_str(), pathString.size() + 1);
			ImGui::EndDragDropSource();
		}

		if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
		{
			if (directoryEntry.is_directory())
				m_CurrentDirectory /= path.filename();

		}

		ImGui::TextWrapped("%s", filenameString.c_str());
		ImGui::NextColumn();

	}

	ImGui::Columns(1);

	ImGui::SliderFloat("Thumbnail Size", &thumbnailSize, 16, 512);
	ImGui::SliderFloat("Padding", &padding, 0, 32);

	ImGui::End();
}



}