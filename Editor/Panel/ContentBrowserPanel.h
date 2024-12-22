#pragma once
#include "Panel.h"

#include <Quark/Core/TimeStep.h>
#include <Quark/Core/FileSystem.h>
#include <Quark/RHI/Common.h>
#include <imgui.h>

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

	Ref<rhi::Image> m_folderIcon;
	Ref<rhi::Image> m_fileIcon;
};
}