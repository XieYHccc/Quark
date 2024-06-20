#pragma once

namespace graphic {

// Forward declaraton
class Device;
class CommandList;
class Buffer;
class Image;
class Shader;
class PipeLine;
class Sampler;

struct RenderPassInfo;
struct BufferDesc;
struct ImageDesc;
struct ImageInitData;
struct ComputePipeLineDesc;
struct GraphicPipeLineDesc;

class GpuResource {
public:
    virtual ~GpuResource() = default;
    GpuResource(const GpuResource&) = delete;
    GpuResource& operator=(const GpuResource&) = delete;
protected:
    GpuResource() = default;
};

enum QueueType
{
    QUEUE_TYPE_GRAPHICS,
    QUEUE_TYPE_ASYNC_COMPUTE,
    QUEUE_TYPE_ASYNC_TRANSFER,
    QUEUE_TYPE_MAX_ENUM,
};

// TODO: Support various image format
enum class DataFormat {
    R8G8B8A8_UNORM,
    B8G8R8A8_UNORM,
    R16G16B16A16_SFLOAT,
    R32G32B32_SFLOAT,
    D32_SFLOAT,
    D32_SFLOAT_S8_UINT,
    D24_UNORM_S8_UINT,
    MAX_ENUM
};

enum class LogicOperation {
    CLEAR,
    AND,
    AND_REVERSE,
    COPY,
    AND_INVERTED,
    NO_OP,
    XOR,
    OR,
    NOR,
    EQUIVALENT,
    INVERT,
    OR_REVERSE,
    COPY_INVERTED,
    OR_INVERTED,
    NAND,
    SET,
    MAX_ENUM
};

enum class CompareOperation {
    NEVER,
    LESS,
    EQUAL,
    LESS_OR_EQUAL,
    GREATER,
    NOT_EQUAL,
    GREATER_OR_EQUAL,
    ALWAYS,
    MAX_ENUM
};

enum class ShaderStage{
    STAGE_COMPUTE,
    STAGE_VERTEX,
    STAGE_FRAGEMNT,
    MAX_ENUM
};

struct Viewport
{
    float x, y, width, height, minDepth, maxDepth;
};

struct Offset
{
    int x, y;
};

struct Extent
{
    unsigned int width, height;
};

struct Scissor
{
    Offset offset;
    Extent extent;
};

constexpr uint32_t GetFormatStride(DataFormat format)
{
    switch (format) {
    case DataFormat::R8G8B8A8_UNORM:
    case DataFormat::D24_UNORM_S8_UINT:
    case DataFormat::D32_SFLOAT:
        return 4u;
    case DataFormat::R32G32B32_SFLOAT:
        return 12u;
    case DataFormat::R16G16B16A16_SFLOAT:
        return 8u;
    default:
    {
        CORE_ASSERT("format not handled yet!")
        return 0u;
    }
    }
}

constexpr bool IsFormatSupportDepth (DataFormat format)
{
    switch (format) {
    case DataFormat::D24_UNORM_S8_UINT:
    case DataFormat::D32_SFLOAT_S8_UINT:
    case DataFormat::D32_SFLOAT:
        return true;
    default:
        return false;
    }
}
}