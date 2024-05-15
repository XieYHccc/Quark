#pragma once
#include "IO.h"

namespace assetlib {

	enum class TextureFormat : uint32_t
	{
		Unknown = 0,
		RGBA8
	};
	
	struct PageInfo {
		uint32_t width;
		uint32_t height;
		uint32_t compressedSize;
		uint32_t originalSize;
	};

	struct TextureInfo {
		uint64_t textureSize;
		TextureFormat textureFormat;
		CompressionMode compressionMode;

		std::string originalFile;
		std::vector<PageInfo> pages;
	};

	//parses the texture metadata from an asset file
	TextureInfo ReadTextureInfo(AssetFile* file);

	void UnpackTexture(TextureInfo* info, const char* sourcebuffer, size_t sourceSize, char* destination);

	AssetFile PackTexture(TextureInfo* info, void* pixelData);
}