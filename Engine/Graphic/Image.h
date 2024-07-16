#pragma once
#include "Common.h"

namespace graphic {
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
    u32 depth = 0;  // for 3d image
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
    
    // For uncompressed format, row_lenght = width.
    // For compressed format, there are maybe paddings in row.
    uint32_t row_length = UINT32_MAX;  // num of pixels per row
    uint32_t image_height = UINT32_MAX; // size of rows
};

class Image : public GpuResource{
public:
    ~Image() = default;
    const ImageDesc& GetDesc() const { return desc_; }
protected:
    Image(const ImageDesc& desc) : desc_(desc) {};
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

class Sampler
{
public:
    virtual ~Sampler() = default;
protected:
    Sampler() = default;
    SamplerDesc desc_;
};

}