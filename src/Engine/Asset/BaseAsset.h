#pragma once
#include "Core/CRC32.h"

using AssetType = u32;
using AssetFilePath = std::filesystem::path;

class BaseAsset
{
public:
    BaseAsset(const std::filesystem::path& assetPath, const std::string assetName)
        : assetPath_(assetPath), assetName_(assetName)
    {
        
    };
    virtual ~BaseAsset() = default;

    virtual u32 GetAssetType() const = 0;

protected:
    AssetFilePath assetPath_;
    std::string assetName_;
};

#define ASSET_TYPE(asset_type)                          \
    static constexpr u32 GetStaticAssetType()           \
    {                                                   \
        return CRC32(asset_type);                       \
    }                                                   \
    u32 GetAssetType() const override                   \
    {                                                   \
        return GetStaticAssetType();                    \
    }
                          