#include "Editor/Panel/ContentBrowserPanel.h"

#include <imgui.h>
#include <Quark/Core/FileSystem.h>

namespace quark {

static std::filesystem::path s_AssetDirectory = "Assets";

ContentBrowserPanel::ContentBrowserPanel()
	:m_CurrentDirectory(s_AssetDirectory)
{

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

	for (auto& directoryEntry : std::filesystem::directory_iterator(m_CurrentDirectory))
	{
		const auto& path = directoryEntry.path();
		std::string filenameString = path.filename().string();

		if (directoryEntry.is_directory())
		{
			if (ImGui::Button(filenameString.c_str()))
			{
				m_CurrentDirectory /= path.filename();
			}
		}
		else
		{
			if (ImGui::Button(filenameString.c_str()))
			{

			}
		}
	}

	ImGui::End();
}



}