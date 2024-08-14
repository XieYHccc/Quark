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
	AssetID id = 0;
	AssetType type = AssetType::None;
	AssetStatus status = AssetStatus::None;

	std::filesystem::path filePath;
	uint64_t fileLastWriteTime = 0; // TODO: this is the last write time of the file WE LOADED
	bool isDataLoaded = false;

	bool IsValid() const { return id != 0; }
};

}