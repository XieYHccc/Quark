#pragma once 
#include "Quark/Asset/Asset.h"
#include "Quark/RHI/TextureFormatLayout.h"

namespace quark {

    struct ImageAsset : public Asset 
    {
        QUARK_ASSET_TYPE_DECL(IMAGE)

        rhi::TextureFormatLayout layout;
        rhi::ImageType type = rhi::ImageType::TYPE_2D;
        rhi::DataFormat format = rhi::DataFormat::R8G8B8A8_UNORM;

        uint32_t width = 0;
        uint32_t height = 0;
        uint32_t depth = 1;
        uint32_t arraySize = 1;
        uint32_t mipLevels = 1;

        std::vector<uint8_t> data;
        std::vector<rhi::ImageInitData> slices;
    };

}