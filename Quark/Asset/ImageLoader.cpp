#include "Asset/ImageLoader.h"
#include <ktx.h>

namespace asset {
Ref<graphic::Image> ImageLoader::LoadKtx(const std::string& file_path) {
    if (file_path.find_last_of(".") != std::string::npos) {
        if (file_path.substr(file_path.find_last_of(".") + 1) != "ktx") {
            CORE_LOGW("ImageLoader::LoadKtx: The file {} is not a ktx file", file_path);
            return nullptr;
        }
    }

    ktxResult result = KTX_SUCCESS;
    ktxTexture* ktxTexture;

    result = ktxTexture_CreateFromNamedFile(file_path.c_str(), KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, &ktxTexture);
    CORE_ASSERT(result == KTX_SUCCESS);

    graphic::ImageDesc desc;
    desc.width = ktxTexture->baseWidth;
    desc.height = ktxTexture->baseHeight;
    desc.depth = ktxTexture->baseDepth;
    desc.mipLevels = ktxTexture->numLevels;
    desc.arraySize = ktxTexture->numLayers;
    desc.initialLayout = graphic::ImageLayout::SHADER_READ_ONLY_OPTIMAL;
    desc.usageBits = graphic::ImageUsageBits::IMAGE_USAGE_SAMPLING_BIT | graphic::ImageUsageBits::IMAGE_USAGE_CAN_COPY_TO_BIT;
    // TODO: Support 3D
    desc.type = graphic::ImageType::TYPE_2D;
    CORE_ASSERT(desc.depth == 1);
    CORE_ASSERT(desc.arraySize == 1);
    //TODO: Support other formats
    desc.format = graphic::DataFormat::R8G8B8A8_UNORM;

    ktx_uint8_t* ktxTextureData = ktxTexture_GetData(ktxTexture);
	ktx_size_t ktxTextureSize = ktxTexture_GetSize(ktxTexture);

    // Cubemap?
    bool isCubeMap = false;
    if (ktxTexture->numLayers == 1 && ktxTexture->numFaces == 6) {
        desc.type = graphic::ImageType::TYPE_CUBE;
        desc.arraySize = 6;
        isCubeMap = true;
    }
    
    //TODO: Support compressed format
    CORE_ASSERT(ktxTexture->isCompressed == KTX_FALSE);

    // Prepare Image's init data
    std::vector<graphic::ImageInitData> initData;
    for (size_t level = 0; level < desc.mipLevels; level++) {
        for (size_t layer = 0; layer < desc.arraySize; layer++) {
            auto& newSubresource = initData.emplace_back();
            ktx_size_t offset;
            KTX_error_code result;
            
            if (isCubeMap)
                result = ktxTexture_GetImageOffset(ktxTexture, level, 0, layer, &offset);
            else 
                result = ktxTexture_GetImageOffset(ktxTexture, level, layer, 0, &offset);

            newSubresource.data = ktxTextureData + offset;
            newSubresource.rowPitch = ktxTexture_GetRowPitch(ktxTexture, level);
            newSubresource.slicePitch = ktxTexture_GetImageSize(ktxTexture, level);
        }
    }

    auto newKtxImage = graphicDevice_->CreateImage(desc, initData.data());
    ktxTexture_Destroy(ktxTexture);

    return newKtxImage;

}

}