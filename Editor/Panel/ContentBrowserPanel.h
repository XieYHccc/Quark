#pragma once
#include <imgui.h>
#include <Quark/Core/TimeStep.h>
#include <Quark/Core/FileSystem.h>
#include <Quark/Asset/Texture.h>
#include "Panel.h"

namespace quark {
class ContentBrowserPanel final : public Panel 
{
public:
	ContentBrowserPanel();
	ContentBrowserPanel(const std::filesystem::path& basePath);

	void Init(const std::filesystem::path& basePath);
	void OnImGuiUpdate() override;

private:
	std::filesystem::path m_currentDirectory;
	std::filesystem::path m_baseDirectory;

	Ref<Texture> m_folderIcon;
	Ref<Texture> m_fileIcon;
};
}