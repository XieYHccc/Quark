#pragma once

// Buffer
struct GPUBufferDesc {

};

struct GPUBuffer {

};

// Image
enum class GPUImageType {
    TYPE_2D,
    TYPE_2D_ARRAY,
    TYPE_CUBE,
    TYPE_CUBE_ARRAY,
};

enum GPUImageUsageBits {
    GPUIMAGE_USAGE_SAMPLING_BIT = (1 << 0),
    GPUIMAGE_USAGE_COLOR_ATTACHMENT_BIT = (1 << 1),
    GPUIMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT = (1 << 2),
    GPUIMAGE_USAGE_STORAGE_BIT = (1 << 3),
    GPUIMAGE_USAGE_CPU_READ_BIT = (1 << 4),
    GPUIMAGE_USAGE_CAN_COPY_FROM_BIT = (1 << 5),
    GPUIMAGE_USAGE_CAN_COPY_TO_BIT = (1 << 6),
    GPUIMAGE_USAGE_INPUT_ATTACHMENT_BIT = (1 << 7),
};

// TODO: Support various image format
enum class GPUImageFormat {
    R8G8B8A8_UNORM,
    D32_SFLOAT,
    D32_SFLOAT_S8_UINT,
    D24_UNORM_S8_UINT
};

enum class GPUImageSamples {
    SAMPLES_1,
    SAMPLES_2,
    SAMPLES_4,
    SAMPLES_8,
    SAMPLES_16,
    SAMPLES_32,
    SAMPLES_64,
};
struct GPUImageDesc {
    u32 width = 0;
    u32 height = 0;
    u32 depth = 0;
    u32 arraySize = 1;
    u32 mipLevels = 1;
    GPUImageType type = GPUImageType::TYPE_2D;
    GPUImageFormat format =  GPUImageFormat::R8G8B8A8_UNORM;
    GPUImageSamples samples = GPUImageSamples::SAMPLES_1;
    u32 usageBits  = 0;
};

struct GPUImage {
    GPUImageDesc desc;
    void* internal;
};

// Render passes