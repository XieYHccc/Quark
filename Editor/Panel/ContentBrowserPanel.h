#pragma once
#include <Quark/Core/TimeStep.h>
#include <Quark/Core/FileSystem.h>
#include "Panel.h"

namespace quark {
class ContentBrowserPanel final : public Panel 
{
public:
	ContentBrowserPanel();

	void OnImGuiUpdate() override;

private:
	std::filesystem::path m_CurrentDirectory;
};
}