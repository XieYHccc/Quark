#pragma once
#include "Quark/RHI/Common.h"

namespace quark::rhi {
enum ImageUsageBits 
{
    IMAGE_USAGE_SAMPLING_BIT = (1 << 0),
    IMAGE_USAGE_COLOR_ATTACHMENT_BIT = (1 << 1),
    IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT = (1 << 2),
    IMAGE_USAGE_STORAGE_BIT = (1 << 3),
    IMAGE_USAGE_CAN_COPY_FROM_BIT = (1 << 4),
    IMAGE_USAGE_CAN_COPY_TO_BIT = (1 << 5),
    IMAGE_USAGE_INPUT_ATTACHMENT_BIT = (1 << 6),
};

enum ImageAspectFlagBits
{
    IMAGE_ASPECT_NONE,
    IMAGE_ASPECT_COLOR_BIT,
    IMAGE_ASPECT_DEPTH_BIT,
    IMAGE_ASPECT_STENCIL_BIT,
    IMAGE_ASPECT_LUMINANCE_BIT,
    IMAGE_ASPECT_CHROMINANCE_BIT,
};

enum class ImageType : uint8_t
{
    TYPE_2D,
    TYPE_3D,
    TYPE_CUBE,
};


enum class ImageLayout : uint8_t
{
    UNDEFINED,
    GENERAL,    // usuallly used for shader read and write
    COLOR_ATTACHMENT_OPTIMAL,
    DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
    DEPTH_STENCIL_READ_ONLY_OPTIMAL,
    SHADER_READ_ONLY_OPTIMAL,
    TRANSFER_SRC_OPTIMAL,
    TRANSFER_DST_OPTIMAL,
    PRESENT
};

enum class SamplerFilter : uint8_t
{
    NEAREST = 1,
    LINEAR = 2
};

enum class SamplerAddressMode : uint8_t
{
    REPEAT = 1,
    MIRRORED_REPEAT = 2,
    CLAMPED_TO_EDGE = 3
};

struct ImageCopySubresourceRange
{
    uint32_t aspect = IMAGE_ASPECT_NONE;
	uint32_t mipLevel = 0;
	uint32_t baseArrayLayer = 0;
	uint32_t layerCount = 1;
};

enum class ImageViewType
{
    TYPE_1D = 0,
    TYPE_2D = 1,
    TYPE_3D = 2,
    TYPE_CUBE = 3,
    TYPE_1D_ARRAY = 4,
    TYPE_2D_ARRAY = 5,
    TYPE_CUBE_ARRAY = 6,
    TYPE_MAX_ENUM = 0x7FFFFFFF
};

struct ImageViewDesc
{
    const Image* image = nullptr;
    DataFormat format = DataFormat::UNDEFINED;
    ImageViewType viewType = ImageViewType::TYPE_MAX_ENUM;
    uint32_t aspect = IMAGE_ASPECT_NONE;
    uint32_t baseLevel = 0;
    uint32_t levelCount = 1;
    uint32_t baseLayer = 0;
    uint32_t layerCount = 1;
};

inline uint32_t FormatToImageAspect(DataFormat format)
{
    switch (format)
    {
    case DataFormat::UNDEFINED:
        return IMAGE_ASPECT_NONE;

    case DataFormat::D32_SFLOAT_S8_UINT:
    case DataFormat::D24_UNORM_S8_UINT:
        return IMAGE_ASPECT_DEPTH_BIT | IMAGE_ASPECT_STENCIL_BIT;
    case DataFormat::D32_SFLOAT:
        return IMAGE_ASPECT_DEPTH_BIT;
    default:
        return IMAGE_ASPECT_COLOR_BIT;
    }
}

class ImageView : GpuResource
{
public:
    virtual ~ImageView() = default;

    const ImageViewDesc& GetDesc() const { return m_desc; }

    GpuResourceType GetGpuResourceType() const override { return GpuResourceType::IMAGE; }

protected:
    ImageView(const ImageViewDesc& desc) : m_desc(desc) {};

    ImageViewDesc m_desc;
};

struct ImageDesc 
{
    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t depth = 1;  // for 3d image
    uint32_t mipLevels = 1;
    uint32_t arraySize = 1;
    uint32_t usageBits  = 0;
    ImageType type = ImageType::TYPE_2D;
    DataFormat format =  DataFormat::R8G8B8A8_UNORM;
    SampleCount samples = SampleCount::SAMPLES_1;
    ImageLayout initialLayout = ImageLayout::UNDEFINED;
    bool generateMipMaps = false;

    static ImageDesc RenderTarget(uint32_t width, uint32_t height, DataFormat format)
    {
        ImageDesc desc;
        desc.width = width;
        desc.height = height;
        desc.depth = 1;
        desc.mipLevels = 1;
        desc.format = format;
        desc.type = ImageType::TYPE_2D;
        desc.arraySize = 1;
        desc.usageBits = (IsFormatSupportDepthOrStencil(format) ? IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT :
            IMAGE_USAGE_COLOR_ATTACHMENT_BIT) | IMAGE_USAGE_CAN_COPY_TO_BIT | IMAGE_USAGE_CAN_COPY_FROM_BIT;
        desc.samples = SampleCount::SAMPLES_1;
        desc.initialLayout = IsFormatSupportDepthOrStencil(format) ? ImageLayout::DEPTH_STENCIL_ATTACHMENT_OPTIMAL :
            ImageLayout::COLOR_ATTACHMENT_OPTIMAL;

        return desc;
    }
};

struct ImageInitData 
{
    const void* data = nullptr;
    // For uncompressed textures the pitch is the number of bytes between rows of texels. 
    // For compressed textures it is the number of bytes between rows of blocks.
    std::uint32_t rowPitch = 0;
    std::uint32_t slicePitch = 0;
};

class Image : public GpuResource
{
public:
    virtual ~Image() = default;
    virtual const ImageView& GetDefaultView() const = 0;
    virtual ImageView& GetDefaultView() = 0;

    const ImageDesc& GetDesc() const { return m_desc; }

    GpuResourceType GetGpuResourceType() const override { return GpuResourceType::IMAGE; }

protected:
    Image(const ImageDesc& desc) : m_desc(desc) {};

    ImageDesc m_desc;

};

struct SamplerDesc
{
    SamplerFilter minFilter;
    SamplerFilter magFliter;
    SamplerAddressMode addressModeU = SamplerAddressMode::REPEAT;
    SamplerAddressMode addressModeV = SamplerAddressMode::REPEAT;
    SamplerAddressMode addressModeW = SamplerAddressMode::REPEAT;
    bool enableAnisotropy = false;

    uint32_t GetHash() const
    {
        uint32_t Id = 0;
        Id += (uint32_t)minFilter * 1;
        Id += (uint32_t)magFliter * 10;
        Id += (uint32_t)addressModeU * 100;
        Id += (uint32_t)addressModeV * 1000;
        Id += (uint32_t)addressModeW * 10000;

        return Id;
    }

    void ResolveHash(uint32_t HashId)
    {
        minFilter = (SamplerFilter)((HashId / 1) % 10);
        magFliter = (SamplerFilter)((HashId / 10) % 10);
        addressModeU = (SamplerAddressMode)((HashId / 100) % 10);
        addressModeV = (SamplerAddressMode)((HashId / 1000) % 10);
        addressModeW = (SamplerAddressMode)((HashId / 10000) % 10);
    };
};

class Sampler : public GpuResource
{
public:
    virtual ~Sampler() = default;

    GpuResourceType GetGpuResourceType() const override { return GpuResourceType::SAMPLER; }

protected:
    Sampler(const SamplerDesc& desc) : m_desc(desc) {};

    SamplerDesc m_desc;
};

}