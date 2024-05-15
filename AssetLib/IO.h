#pragma once

#include <vector>
#include <string>

namespace assetlib {

struct AssetFile {
    char type[4];
    int version;
    std::string json;
    std::vector<char> binaryBlob;
};

enum class CompressionMode : uint32_t {
    None,
    LZ4
};

bool SaveBinaryFile(const char* path, const AssetFile& file);

bool LoadBinaryFile(const char* path, AssetFile& outputFile);	

CompressionMode ParseCompression(const char* f);
}