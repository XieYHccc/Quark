#pragma once
#include "Quark/Graphic/Common.h"

namespace quark::graphic {
enum ImageUsageBits {
    IMAGE_USAGE_SAMPLING_BIT = (1 << 0),
    IMAGE_USAGE_COLOR_ATTACHMENT_BIT = (1 << 1),
    IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT = (1 << 2),
    IMAGE_USAGE_STORAGE_BIT = (1 << 3),
    IMAGE_USAGE_CAN_COPY_FROM_BIT = (1 << 4),
    IMAGE_USAGE_CAN_COPY_TO_BIT = (1 << 5),
    IMAGE_USAGE_INPUT_ATTACHMENT_BIT = (1 << 6),
};

enum class ImageType {
    TYPE_2D,
    TYPE_3D,
    TYPE_CUBE,
};

enum class ImageLayout {
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

enum class SamplerFilter
{
    NEAREST = 1,
    LINEAR = 2
};

enum class SamplerAddressMode
{
    REPEAT = 1,
    MIRRORED_REPEAT = 2,
    CLAMPED_TO_EDGE = 3
};

struct ImageDesc {
    u32 width = 0;
    u32 height = 0;
    u32 depth = 1;  // for 3d image
    u32 mipLevels = 1;
    u32 arraySize = 1;
    ImageType type = ImageType::TYPE_2D;
    DataFormat format =  DataFormat::R8G8B8A8_UNORM;
    SampleCount samples = SampleCount::SAMPLES_1;
    ImageLayout initialLayout = ImageLayout::UNDEFINED;
    u32 usageBits  = 0;
    bool generateMipMaps = false;
};

struct ImageInitData {
    const void* data = nullptr;
    // For uncompressed textures the pitch is the number of bytes between rows of texels. 
    // For compressed textures it is the number of bytes between rows of blocks.
    std::uint32_t rowPitch = 0;
    std::uint32_t slicePitch = 0;
};

class Image : public GpuResource{
public:
    ~Image() = default;
    Image(const ImageDesc& desc) : desc_(desc) {};

    const ImageDesc& GetDesc() const { return desc_; }

    GPU_RESOURCE_TYPE GetGpuResourceType() const override { return GPU_RESOURCE_TYPE::IMAGE; }

protected:
    ImageDesc desc_;

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
    Sampler() = default;
    virtual ~Sampler() = default;

    GPU_RESOURCE_TYPE GetGpuResourceType() const override { return GPU_RESOURCE_TYPE::SAMPLER; }

private:
    SamplerDesc desc_;
};

}