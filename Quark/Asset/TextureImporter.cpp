#include "Quark/qkpch.h"
#include "Quark/Asset/TextureImporter.h"
#include "Quark/Core/Application.h"
#include "Quark/Core/FileSystem.h"
#include "Quark/RHI/TextureFormatLayout.h"
#include "Quark/Renderer/Renderer.h"

#include <ktx.h>
#include <basisu_transcoder.h>
#include <stb_image.h>

namespace quark {
Ref<Texture> TextureImporter::ImportKtx2(const std::string &file_path, bool isCubemap)
{
    if (file_path.find_last_of(".") != std::string::npos) 
    {
        if (file_path.substr(file_path.find_last_of(".") + 1) != "ktx2") 
        {
            QK_CORE_LOGW_TAG("AssetManager", "TextureImporter::LoadKtx: The file{} is not a ktx2 file", file_path);
            return nullptr;
        }
    }

    // Read in file's binary data
    std::vector<byte> binary_data;
    if (!FileSystem::ReadFileBytes(file_path, binary_data)) 
    {
        QK_CORE_LOGW_TAG("AssetManager", "TextureImporter::LoadKtx2: Failed to read file {}", file_path);
        return nullptr;
    }

    static bool basis_init = false; // within lock!
    if (!basis_init) {
        basis_init = true;
        basist::basisu_transcoder_init();
    }

    // Init ktx2 transcoder
    basist::ktx2_transcoder ktxTranscoder;
    bool success = ktxTranscoder.init(binary_data.data(), (uint32_t)binary_data.size());
    if (!success) 
    {
        QK_CORE_LOGW_TAG("AssetManager", "TextureImporter::LoadKtx2: Failed to initialize ktx2 transcoder for file {}", file_path);
        return nullptr;
    }

    // Image description
    rhi::ImageDesc desc;
    desc.width = ktxTranscoder.get_width();
    desc.height = ktxTranscoder.get_height();
    desc.arraySize = std::max(1u, ktxTranscoder.get_layers());
    desc.mipLevels = ktxTranscoder.get_levels();
    desc.format = rhi::DataFormat::R8G8B8A8_UNORM;
    desc.initialLayout = rhi::ImageLayout::SHADER_READ_ONLY_OPTIMAL;
    desc.type = rhi::ImageType::TYPE_2D;
    desc.usageBits = rhi::ImageUsageBits::IMAGE_USAGE_SAMPLING_BIT | rhi::ImageUsageBits::IMAGE_USAGE_CAN_COPY_TO_BIT;
    if (ktxTranscoder.get_faces() == 6) {
        desc.type = rhi::ImageType::TYPE_CUBE;
        desc.arraySize = desc.arraySize * 6;
    }

    // Find a suitable format
    desc.format = rhi::DataFormat::R8G8B8A8_UNORM;
    basist::transcoder_texture_format targetFormat = basist::transcoder_texture_format::cTFRGBA32;

    auto* graphicDevice = Application::Get().GetGraphicDevice();
    if (graphicDevice->GetDeviceFeatures().textureCompressionBC) 
    {
        // BC7 is the preferred block compression if available
        if (graphicDevice->isFormatSupported(rhi::DataFormat::BC7_UNORM_BLOCK)) {
            targetFormat = basist::transcoder_texture_format::cTFBC7_RGBA;
            desc.format = rhi::DataFormat::BC7_UNORM_BLOCK;
        } 
        else 
        {
            if (graphicDevice->isFormatSupported(rhi::DataFormat::BC3_UNORM_BLOCK)) {
                targetFormat = basist::transcoder_texture_format::cTFBC3_RGBA;
                desc.format = rhi::DataFormat::BC3_UNORM_BLOCK;
            }
        }
    }

    // Transcode data
    if (ktxTranscoder.start_transcoding()) 
    {
        const uint32_t layers = std::max(1u, ktxTranscoder.get_layers());
        const uint32_t faces = ktxTranscoder.get_faces();
        const uint32_t levels = ktxTranscoder.get_levels();

        // Setup mip level infos : we use our own function to get mipmap infos for algning offset with 16 bytes
        rhi::TextureFormatLayout layout;
        layout.SetUp2D(desc.format, desc.width, desc.height, desc.arraySize, desc.mipLevels);

        const uint32_t bytesPerBlockOrPixel = layout.GetBlockStride();
        std::vector<uint8_t> transcodedData(layout.GetRequiredSize());
        std::vector<rhi::ImageInitData> initData;
        for (uint32_t level = 0; level < levels; ++level) {
            const auto mipInfo = layout.GetMipInfo(level);
            const uint32_t numBlocksOrPixels = mipInfo.num_block_x * mipInfo.num_block_y;
            const uint32_t slicePitch = numBlocksOrPixels * bytesPerBlockOrPixel;

            for (uint32_t layer = 0; layer < layers; ++layer) {
                for (uint32_t face = 0; face < faces; ++face) {
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

        if (!initData.empty()) 
        {
            Ref<Texture> newTexture = CreateRef<Texture>();
            newTexture->image = graphicDevice->CreateImage(desc, initData.data());
            newTexture->sampler = isCubemap? Renderer::Get().sampler_cube : Renderer::Get().sampler_linear;
            ktxTranscoder.clear();
            return newTexture;
        }

    }

    return nullptr;
}

Ref<Texture> TextureImporter::ImportStb(const std::string& file_path)
{
    using namespace rhi;

    int width, height, channels;
    // stbi_set_flip_vertically_on_load(true);
    void* data = stbi_load(file_path.c_str(), &width, &height, &channels, STBI_rgb_alpha);
    if (!data) 
    {
        QK_CORE_LOGW_TAG("AssetManager", "TextureImporter::LoadStb: Failed to load image {}", file_path);
		return nullptr;
	}

    ImageDesc desc;
    desc.width = static_cast<u32>(width);
    desc.height = static_cast<u32>(height);
    desc.depth = 1u;
    desc.arraySize = 1;     // Only support 1 layer and 1 mipmap level for embedded image
    desc.mipLevels = 1;
    desc.format = DataFormat::R8G8B8A8_UNORM;
    desc.type = ImageType::TYPE_2D;
    desc.usageBits = IMAGE_USAGE_SAMPLING_BIT | IMAGE_USAGE_CAN_COPY_TO_BIT | IMAGE_USAGE_CAN_COPY_FROM_BIT;
    desc.initialLayout = ImageLayout::SHADER_READ_ONLY_OPTIMAL;
    desc.generateMipMaps = false;        // Generate mipmaps for embedded image

    ImageInitData init_data;
    init_data.data = data;
    init_data.rowPitch = desc.width * 4;
    init_data.slicePitch = init_data.rowPitch * desc.height;

    Ref<Texture> newTexture = CreateRef<Texture>();
    newTexture->image = Application::Get().GetGraphicDevice()->CreateImage(desc, &init_data);
    newTexture->sampler = Renderer::Get().sampler_linear;

    QK_CORE_LOGI_TAG("AssetManager", "TextureImporter: Import texture asset: {0}", file_path);

    return newTexture;
}


Ref<Texture> TextureImporter::ImportKtx(const std::string& file_path, bool isCubemap) {
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

    rhi::ImageDesc desc;
    desc.width = ktxTexture->baseWidth;
    desc.height = ktxTexture->baseHeight;
    desc.depth = ktxTexture->baseDepth;
    desc.mipLevels = ktxTexture->numLevels;
    desc.arraySize = ktxTexture->numLayers;
    desc.initialLayout = rhi::ImageLayout::SHADER_READ_ONLY_OPTIMAL;
    desc.usageBits = rhi::ImageUsageBits::IMAGE_USAGE_SAMPLING_BIT | rhi::ImageUsageBits::IMAGE_USAGE_CAN_COPY_TO_BIT;
    // TODO: Support 3D
    desc.type = rhi::ImageType::TYPE_2D;
    QK_CORE_VERIFY(desc.depth == 1);
    QK_CORE_VERIFY(desc.arraySize == 1);
    //TODO: Support other formats
    desc.format = rhi::DataFormat::R8G8B8A8_UNORM;

    ktx_uint8_t* ktxTextureData = ktxTexture_GetData(ktxTexture);
	ktx_size_t ktxTextureSize = ktxTexture_GetSize(ktxTexture);

    // Cubemap?
    bool isCubeMap = false;
    if (ktxTexture->numLayers == 1 && ktxTexture->numFaces == 6) {
        desc.type = rhi::ImageType::TYPE_CUBE;
        desc.arraySize = 6;
        isCubeMap = true;
    }
    
    //TODO: Support compressed format
    QK_CORE_VERIFY(ktxTexture->isCompressed == KTX_FALSE);

    // Prepare Image's init data
    std::vector<rhi::ImageInitData> initData;
    for (uint32_t level = 0; level < desc.mipLevels; level++) {
        for (uint32_t layer = 0; layer < desc.arraySize; layer++) {
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

    Ref<Texture> newTexture = CreateRef<Texture>();
    newTexture->image = Application::Get().GetGraphicDevice()->CreateImage(desc, initData.data());
    newTexture->sampler = isCubeMap ? Renderer::Get().sampler_cube : Renderer::Get().sampler_linear;

    return newTexture;
}

}