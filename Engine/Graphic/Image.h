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

enum class ImageSamples {
    SAMPLES_1,
    SAMPLES_2,
    SAMPLES_4,
    SAMPLES_8,
    SAMPLES_16,
    SAMPLES_32,
    SAMPLES_64,
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

enum class SamplerWrapMode
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
    ImageSamples samples = ImageSamples::SAMPLES_1;
    ImageLayout initialLayout = ImageLayout::UNDEFINED;
    u32 usageBits  = 0;
};

struct ImageInitData {
    const void* data = nullptr;
    uint32_t image_width = 0;
    uint32_t image_heigt = 0;
};

class Image : public GpuResource{
public:
    ~Image() = default;
    const ImageDesc& GetDesc() const { return desc_; }
protected:
    Image(const ImageDesc& desc) : desc_(desc) {};
    ImageDesc desc_;

};

struct SamplerSpecification
{
    SamplerFilter nearFilter;
    SamplerFilter farFliter;
    SamplerWrapMode wrapModeU, wrapModeV, wrapModeW;

    uint32_t GetHash() const
    {
        uint32_t Id = 0;
        Id += (uint32_t)farFliter * 1;
        Id += (uint32_t)nearFilter * 10;
        Id += (uint32_t)wrapModeU * 100;
        Id += (uint32_t)wrapModeV * 1000;
        Id += (uint32_t)wrapModeW * 10000;

        return Id;
    }

    void ResolveHash(uint32_t HashId)
    {
        farFliter = (SamplerFilter)((HashId / 1) % 10);
        nearFilter = (SamplerFilter)((HashId / 10) % 10);
        wrapModeU = (SamplerWrapMode)((HashId / 100) % 10);
        wrapModeV = (SamplerWrapMode)((HashId / 1000) % 10);
        wrapModeW = (SamplerWrapMode)((HashId / 10000) % 10);
    };
};

class Sampler
{
public:
    virtual ~Sampler() = default;
protected:
    Sampler() = default;
};

}