#include "Quark/qkpch.h"
#include "Quark/Asset/TextureLoader.h"

#include <ktx.h>
#include <basisu_transcoder.h>
#include <stb_image.h>

#include "Quark/Core/Application.h"
#include "Quark/Core/FileSystem.h"
#include "Quark/Graphic/TextureFormatLayout.h"
#include "Quark/Renderer/DefaultRenderResources.h"

namespace quark {
Ref<Texture> TextureLoader::LoadKtx2(const std::string &file_path, bool isCubemap)
{
    if (file_path.find_last_of(".") != std::string::npos) {
        if (file_path.substr(file_path.find_last_of(".") + 1) != "ktx2") {
            CORE_LOGW("TextureLoader::LoadKtx: The file {} is not a ktx2 file", file_path);
            return nullptr;
        }
    }

    // Read in file's binary data
    std::vector<byte> binary_data;
    if (!FileSystem::ReadFile(file_path, binary_data)) {
        CORE_LOGW("TextureLoader::LoadKtx2: Failed to read file {}", file_path);
        return nullptr;
    }

    static bool basis_init = false; // within lock!
    if (!basis_init) {
        basis_init = true;
        basist::basisu_transcoder_init();
    }

    // Init ktx2 transcoder
    basist::ktx2_transcoder ktxTranscoder;
    bool success = ktxTranscoder.init(binary_data.data(), binary_data.size());
    if (!success) {
        CORE_LOGW("TextureLoader::LoadKtx2: Failed to initialize ktx2 transcoder for file {}", file_path);
        return nullptr;
    }

    // Image description
    graphic::ImageDesc desc;
    desc.width = ktxTranscoder.get_width();
    desc.height = ktxTranscoder.get_height();
    desc.arraySize = std::max(1u, ktxTranscoder.get_layers());
    desc.mipLevels = ktxTranscoder.get_levels();
    desc.format = graphic::DataFormat::R8G8B8A8_UNORM;
    desc.initialLayout = graphic::ImageLayout::SHADER_READ_ONLY_OPTIMAL;
    desc.type = graphic::ImageType::TYPE_2D;
    desc.usageBits = graphic::ImageUsageBits::IMAGE_USAGE_SAMPLING_BIT | graphic::ImageUsageBits::IMAGE_USAGE_CAN_COPY_TO_BIT;
    if (ktxTranscoder.get_faces() == 6) {
        desc.type = graphic::ImageType::TYPE_CUBE;
        desc.arraySize = desc.arraySize * 6;
    }

    // Find a suitable format
    desc.format = graphic::DataFormat::R8G8B8A8_UNORM;
    basist::transcoder_texture_format targetFormat = basist::transcoder_texture_format::cTFRGBA32;

    auto* graphicDevice = Application::Get().GetGraphicDevice();
    if (graphicDevice->features.textureCompressionBC) {
        // BC7 is the preferred block compression if available
        if (graphicDevice->isFormatSupported(graphic::DataFormat::BC7_UNORM_BLOCK)) {
            targetFormat = basist::transcoder_texture_format::cTFBC7_RGBA;
            desc.format = graphic::DataFormat::BC7_UNORM_BLOCK;
        } 
        else {
            if (graphicDevice->isFormatSupported(graphic::DataFormat::BC3_UNORM_BLOCK)) {
                targetFormat = basist::transcoder_texture_format::cTFBC3_RGBA;
                desc.format = graphic::DataFormat::BC3_UNORM_BLOCK;
            }
        }
    }

    // Transcode data
    if (ktxTranscoder.start_transcoding()) {
        const uint32_t layers = std::max(1u, ktxTranscoder.get_layers());
        const uint32_t faces = ktxTranscoder.get_faces();
        const uint32_t levels = ktxTranscoder.get_levels();

        // Setup mip level infos : we use our own function to get mipmap infos for algning offset with 16 bytes
        graphic::TextureFormatLayout layout;
        layout.SetUp2D(desc.format, desc.width, desc.height, desc.arraySize, desc.mipLevels);

        const uint32_t bytesPerBlockOrPixel = layout.GetBlockStride();
        std::vector<uint8_t> transcodedData(layout.GetRequiredSize());
        std::vector<graphic::ImageInitData> initData;
        for (uint32_t level = 0; level < levels; ++level) {
            const auto mipInfo = layout.GetMipInfo(level);
            const uint32_t numBlocksOrPixels = mipInfo.num_block_x * mipInfo.num_block_y;
            const uint32_t slicePitch = numBlocksOrPixels * bytesPerBlockOrPixel;

            for (uint32_t layer = 0; layer < layers; ++layer) {
                for (uint32_t face = 0; face < faces; ++face) {
                    void* pDst = transcodedData.data() + mipInfo.offset + (slicePitch * faces) * layer + slicePitch * face;

                    if (ktxTranscoder.transcode_image_level(level, layer, face, pDst, numBlocksOrPixels, targetFormat)) {
                        graphic::ImageInitData newSubresource;
                        newSubresource.data = pDst;
                        newSubresource.rowPitch = mipInfo.num_block_x * bytesPerBlockOrPixel;
                        newSubresource.slicePitch = newSubresource.rowPitch * mipInfo.num_block_y;
                        initData.push_back(newSubresource);
                    } 
                    else {
                        CORE_LOGW("TextureLoader::LoadKtx2: Failed to transcode image level {} layer {} face {}", level, layer, face);
                        return nullptr;
                    }

                }
            }
        }

        if (!initData.empty()) 
        {
            Ref<Texture> newTexture = CreateRef<Texture>();
            newTexture->image = graphicDevice->CreateImage(desc, initData.data());
            newTexture->sampler = isCubemap? DefaultRenderResources::cubeMapSampler : DefaultRenderResources::linearSampler;
            ktxTranscoder.clear();
            return newTexture;
        }

    }

    return nullptr;
}

Ref<Texture> TextureLoader::LoadStb(const std::string& file_path)
{
    using namespace graphic;

    int width, height, channels;
    // stbi_set_flip_vertically_on_load(true);
    void* data = stbi_load(file_path.c_str(), &width, &height, &channels, STBI_rgb_alpha);
    if (!data) 
    {
		CORE_LOGW("TextureLoader::LoadStb: Failed to load image {}", file_path);
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
    newTexture->sampler = DefaultRenderResources::linearSampler;

    return newTexture;
}


Ref<graphic::Image> TextureLoader::LoadKtx(const std::string& file_path) {
    if (file_path.find_last_of(".") != std::string::npos) {
        std::string file_extension = file_path.substr(file_path.find_last_of(".") + 1);
        if (file_extension != "ktx" && file_extension != "ktx2") {
            CORE_LOGW("TextureLoader::LoadKtx: The file {} is not a ktx file", file_path);
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

    auto newKtxImage = Application::Get().GetGraphicDevice()->CreateImage(desc, initData.data());
    ktxTexture_Destroy(ktxTexture);

    return newKtxImage;

}

}