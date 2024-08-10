#pragma once
#include <filesystem>
#include "Quark/Asset/Asset.h"

namespace quark {

enum class AssetStatus
{
	None = 0, Ready = 1, Invalid = 2, Loading = 3
};

struct AssetMetadata
{
	AssetID m_Id = 0;
	AssetType m_Type = AssetType::MAX_ENUM;
	AssetStatus m_Status = AssetStatus::None;

	std::filesystem::path m_FilePath;
	uint64_t m_FileLastWriteTime = 0; // TODO: this is the last write time of the file WE LOADED
	bool m_IsDataLoaded = false;

	bool IsValid() const { return m_Id != 0; }
};

}