#include "Quark/qkpch.h"
#include "Quark/Asset/ImageImporter.h"
#include "Quark/Core/FileSystem.h"
#include "Quark/Core/Application.h"

#include <ktx.h>
#include <basisu_transcoder.h>
#include <stb_image.h>

namespace quark 
{
    
    Ref<ImageAsset> ImageImporter::Import(const std::string& file_path)
    {
        if (file_path.find_last_of(".") != std::string::npos) 
        {
            if (file_path.substr(file_path.find_last_of(".") + 1) == "ktx") 
            {
                return ImportKtx(file_path);
            }
            else if (file_path.substr(file_path.find_last_of(".") + 1) == "ktx2") 
            {
                return ImportKtx2(file_path);
            }
            else 
            {
                return ImportStb(file_path);
            }
        }

        QK_CORE_LOGW_TAG("AssetManager", "ImageAssetImporter::Import: Unsupported file format for file {}", file_path);
        return nullptr;
    }

    Ref<ImageAsset> ImageImporter::ImportKtx(const std::string& file_path, bool isCubemap)
    {
        if (file_path.find_last_of(".") != std::string::npos) {
            std::string file_extension = file_path.substr(file_path.find_last_of(".") + 1);
            if (file_extension != "ktx" && file_extension != "ktx2")
            {
                QK_CORE_LOGW_TAG("AssetManager", "TextureImporter::LoadKtx: The file {} is not a ktx file", file_path);
                return nullptr;
            }
        }

        ktxResult result = KTX_SUCCESS;
        ktxTexture* ktxTexture;

        result = ktxTexture_CreateFromNamedFile(file_path.c_str(), KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, &ktxTexture);
        QK_CORE_VERIFY(result == KTX_SUCCESS);

        Ref<ImageAsset> newImage = CreateRef<ImageAsset>();
        newImage->width = ktxTexture->baseWidth;
        newImage->height = ktxTexture->baseHeight;
        newImage->depth = ktxTexture->baseDepth;
        newImage->mipLevels = ktxTexture->numLevels;
        newImage->arraySize = ktxTexture->numLayers;

        // TODO: Support 3D
        newImage->type = rhi::ImageType::TYPE_2D;
        //TODO: Support other formats
        newImage->format = rhi::DataFormat::R8G8B8A8_UNORM;

        ktx_uint8_t* ktxTextureData = ktxTexture_GetData(ktxTexture);
        ktx_size_t ktxTextureSize = ktxTexture_GetSize(ktxTexture);

        // Cubemap?
        bool isCubeMap = false;
        if (ktxTexture->numLayers == 1 && ktxTexture->numFaces == 6) {
            newImage->type = rhi::ImageType::TYPE_CUBE;
            newImage->arraySize = 6;
            isCubeMap = true;
        }

        //TODO: Support compressed format
        QK_CORE_VERIFY(ktxTexture->isCompressed == KTX_FALSE);

        // Prepare Image's init data
        std::vector<rhi::ImageInitData> initData;
        for (uint32_t level = 0; level < newImage->mipLevels; level++) {
            for (uint32_t layer = 0; layer < newImage->arraySize; layer++) {
                auto& newSubresource = initData.emplace_back();
                ktx_size_t offset;
                KTX_error_code result;

                if (isCubeMap)
                    result = ktxTexture_GetImageOffset(ktxTexture, level, 0, layer, &offset);
                else
                    result = ktxTexture_GetImageOffset(ktxTexture, level, layer, 0, &offset);

                newSubresource.data = ktxTextureData + offset;
                newSubresource.rowPitch = ktxTexture_GetRowPitch(ktxTexture, level);
                newSubresource.slicePitch = (uint32_t)ktxTexture_GetImageSize(ktxTexture, level);
            }
        }

        return newImage;
    }

    Ref<ImageAsset> ImageImporter::ImportKtx2(const std::string& file_path, bool isCubemap)
    {
        
        if (file_path.find_last_of(".") != std::string::npos) 
        {
            if (file_path.substr(file_path.find_last_of(".") + 1) != "ktx2") 
            {
                QK_CORE_LOGW_TAG("AssetManager", "TextureImporter::LoadKtx: The file{} is not a ktx2 file", file_path);
                return nullptr;
            }
        }

        // read in file's binary data
        std::vector<byte> binary_data;
        if (!FileSystem::ReadFileBytes(file_path, binary_data)) 
        {
            QK_CORE_LOGW_TAG("AssetManager", "TextureImporter::LoadKtx2: Failed to read file {}", file_path);
            return nullptr;
        }

        static bool basis_init = false; // within lock!
        if (!basis_init) 
        {
            basis_init = true;
            basist::basisu_transcoder_init();
        }

        // init ktx2 transcoder
        basist::ktx2_transcoder ktxTranscoder;
        bool success = ktxTranscoder.init(binary_data.data(), (uint32_t)binary_data.size());
        if (!success) 
        {
            QK_CORE_LOGW_TAG("AssetManager", "TextureImporter::LoadKtx2: Failed to initialize ktx2 transcoder for file {}", file_path);
            return nullptr;
        }

        Ref<ImageAsset> new_image_asset = CreateRef<ImageAsset>();
        new_image_asset->width = ktxTranscoder.get_width();
        new_image_asset->height = ktxTranscoder.get_height();
        new_image_asset->arraySize = std::max(1u, ktxTranscoder.get_layers());
        new_image_asset->mipLevels = ktxTranscoder.get_levels();
        new_image_asset->type = rhi::ImageType::TYPE_2D;
        // cubemaps?
        if (ktxTranscoder.get_faces() == 6)
        {
            new_image_asset->type = rhi::ImageType::TYPE_CUBE;
            new_image_asset->arraySize = new_image_asset->arraySize * 6;
        }

        // find a suitable format
        new_image_asset->format = rhi::DataFormat::R8G8B8A8_UNORM;
        basist::transcoder_texture_format targetFormat = basist::transcoder_texture_format::cTFRGBA32;

        auto graphicDevice = Application::Get().GetGraphicDevice();
        if (graphicDevice->GetDeviceFeatures().textureCompressionBC) 
        {
            // BC7 is the preferred block compression if available
            if (graphicDevice->isFormatSupported(rhi::DataFormat::BC7_UNORM_BLOCK)) 
            {
                targetFormat = basist::transcoder_texture_format::cTFBC7_RGBA;
                new_image_asset->format = rhi::DataFormat::BC7_UNORM_BLOCK;
            } 
            else 
            {
                if (graphicDevice->isFormatSupported(rhi::DataFormat::BC3_UNORM_BLOCK)) 
                {
                    targetFormat = basist::transcoder_texture_format::cTFBC3_RGBA;
                    new_image_asset->format = rhi::DataFormat::BC3_UNORM_BLOCK;
                }
            }
        }

        // transcode data
        if (ktxTranscoder.start_transcoding()) 
        {
            const uint32_t layers = std::max(1u, ktxTranscoder.get_layers());
            const uint32_t faces = ktxTranscoder.get_faces();
            const uint32_t levels = ktxTranscoder.get_levels();

            // Setup mip level infos : we use our own function to get mipmap infos for algning offset with 16 bytes
            rhi::TextureFormatLayout& layout = new_image_asset->layout;
            layout.SetUp2D(new_image_asset->format, new_image_asset->width, new_image_asset->height, new_image_asset->arraySize, new_image_asset->mipLevels); 

            const uint32_t bytesPerBlockOrPixel = layout.GetBlockStride();
            std::vector<uint8_t> transcodedData(layout.GetRequiredSize());
            std::vector<rhi::ImageInitData> initData;
            for (uint32_t level = 0; level < levels; ++level) 
            {
                const auto mipInfo = layout.GetMipInfo(level);
                const uint32_t numBlocksOrPixels = mipInfo.num_block_x * mipInfo.num_block_y;
                const uint32_t slicePitch = numBlocksOrPixels * bytesPerBlockOrPixel;

                for (uint32_t layer = 0; layer < layers; ++layer) 
                {
                    for (uint32_t face = 0; face < faces; ++face) 
                    {
                        void* pDst = transcodedData.data() + mipInfo.offset + (slicePitch * faces) * layer + slicePitch * face;

                        if (ktxTranscoder.transcode_image_level(level, layer, face, pDst, numBlocksOrPixels, targetFormat)) {
                            rhi::ImageInitData newSubresource;
                            newSubresource.data = pDst;
                            newSubresource.rowPitch = mipInfo.num_block_x * bytesPerBlockOrPixel;
                            newSubresource.slicePitch = newSubresource.rowPitch * mipInfo.num_block_y;
                            initData.push_back(newSubresource);
                        } 
                        else {
                            QK_CORE_LOGW_TAG("AssetManager", "TextureImporter::LoadKtx2: Failed to transcode image level {} layer {} face {}", level, layer, face);
                            return nullptr;
                        }
                    }
                }
            }

            new_image_asset->data = std::move(transcodedData);
            new_image_asset->slices = std::move(initData);

        }

        ktxTranscoder.clear();
        return new_image_asset;
    }

    Ref<ImageAsset> ImageImporter::ImportStb(const std::string& file_path)
    {
        int width, height, channels;

        // stbi_set_flip_vertically_on_load(true);
        void* data = stbi_load(file_path.c_str(), &width, &height, &channels, STBI_rgb_alpha);
        if (!data) 
        {
            QK_CORE_LOGW_TAG("ImageAssetImporter", "TextureImporter::LoadStb: Failed to load image {}", file_path);
            return nullptr;
        }

        Ref<ImageAsset> new_image_asset = CreateRef<ImageAsset>();
        new_image_asset->width = static_cast<u32>(width);
        new_image_asset->height = static_cast<u32>(height);
        new_image_asset->depth = 1u;
        new_image_asset->arraySize = 1;     // Only support 1 layer and 1 mipmap level for embedded image
        new_image_asset->mipLevels = 1;
        new_image_asset->format = rhi::DataFormat::R8G8B8A8_UNORM;
        new_image_asset->type = rhi::ImageType::TYPE_2D;
        new_image_asset->data.resize(width * height * 4);
        memcpy(new_image_asset->data.data(), data, width * height * 4);

        rhi::ImageInitData init_data;
        init_data.data = data;
        init_data.rowPitch = width * 4;
        init_data.slicePitch = init_data.rowPitch * height;
        new_image_asset->slices.push_back(init_data);

        QK_CORE_LOGI_TAG("ImageAssetImporter", "ImageAssetImporter: Import Image asset: {0}", file_path);

        return new_image_asset;
    }
}