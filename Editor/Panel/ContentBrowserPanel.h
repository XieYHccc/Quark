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

	void OnImGuiUpdate() override;

private:
	std::filesystem::path m_CurrentDirectory;

	Ref<Texture> m_FolderIcon;
	Ref<Texture> m_FileIcon;
	ImTextureID m_FolderIconId;
	ImTextureID m_FileIconId;
};
}