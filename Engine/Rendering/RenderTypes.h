#pragma once
#include<glm/glm.hpp>

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

enum PipelineStageBits {
    PIPELINE_STAGE_TOP_OF_PIPE_BIT = (1 << 0),
    PIPELINE_STAGE_DRAW_INDIRECT_BIT = (1 << 1),
    PIPELINE_STAGE_VERTEX_INPUT_BIT = (1 << 2),
    PIPELINE_STAGE_VERTEX_SHADER_BIT = (1 << 3),
    PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT = (1 << 4),
    PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT = (1 << 5),
    PIPELINE_STAGE_GEOMETRY_SHADER_BIT = (1 << 6),
    PIPELINE_STAGE_FRAGMENT_SHADER_BIT = (1 << 7),
    PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT = (1 << 8),
    PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT = (1 << 9),
    PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT = (1 << 10),
    PIPELINE_STAGE_COMPUTE_SHADER_BIT = (1 << 11),
    PIPELINE_STAGE_TRANSFER_BIT = (1 << 12),
    PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT = (1 << 13),
    PIPELINE_STAGE_ALL_GRAPHICS_BIT = (1 << 15),
    PIPELINE_STAGE_ALL_COMMANDS_BIT = (1 << 16),
};

enum PipelineMemoryAccessBits {
    BARRIER_ACCESS_INDIRECT_COMMAND_READ_BIT = (1 << 0),
    BARRIER_ACCESS_INDEX_READ_BIT = (1 << 1),
    BARRIER_ACCESS_VERTEX_ATTRIBUTE_READ_BIT = (1 << 2),
    BARRIER_ACCESS_UNIFORM_READ_BIT = (1 << 3),
    BARRIER_ACCESS_INPUT_ATTACHMENT_READ_BIT = (1 << 4),
    BARRIER_ACCESS_SHADER_READ_BIT = (1 << 5),
    BARRIER_ACCESS_SHADER_WRITE_BIT = (1 << 6),
    BARRIER_ACCESS_COLOR_ATTACHMENT_READ_BIT = (1 << 7),
    BARRIER_ACCESS_COLOR_ATTACHMENT_WRITE_BIT = (1 << 8),
    BARRIER_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT = (1 << 9),
    BARRIER_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT = (1 << 10),
    BARRIER_ACCESS_TRANSFER_READ_BIT = (1 << 11),
    BARRIER_ACCESS_TRANSFER_WRITE_BIT = (1 << 12),
    BARRIER_ACCESS_HOST_READ_BIT = (1 << 13),
    BARRIER_ACCESS_HOST_WRITE_BIT = (1 << 14),
    BARRIER_ACCESS_MEMORY_READ_BIT = (1 << 15),
    BARRIER_ACCESS_MEMORY_WRITE_BIT = (1 << 16),
};

enum class GPUImageType {
    TYPE_2D,
    TYPE_2D_ARRAY,
    TYPE_CUBE,
    TYPE_CUBE_ARRAY,
    TYPE_MAX
};

// TODO: Support various image format
enum class DataFormat {
    R8G8B8A8_UNORM,
    R32G32B32_SFLOAT,
    D32_SFLOAT,
    D32_SFLOAT_S8_UINT,
    D24_UNORM_S8_UINT,
    MAX_ENUM
};

enum class GPUImageSamples {
    SAMPLES_1,
    SAMPLES_2,
    SAMPLES_4,
    SAMPLES_8,
    SAMPLES_16,
    SAMPLES_32,
    SAMPLES_64,
    MAX_ENUM
};

enum class ShaderStage{
    STAGE_COMPUTE,
    STAGE_VERTEX,
    STAGE_FRAGEMNT,
    MAX_ENUM
};

enum class PrimitiveTopology {
    TRANGLE_LIST,
    LINE_LIST,
    POINT_LIST,
    TRIANGLE_STRIPS_WITH_RESTART_INDEX,
    MAX_ENUM
};


enum class CullMode{
    NONE,
    FRONT,
    BACK,
};

enum class BlendOperation {
    ADD,
    SUBTRACT,
    REVERSE_SUBTRACT,
    MINIMUM,
    MAXIMUM, // Yes, this one is an actual operator.
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
enum class BlendFactor {
    ZERO,
    ONE,
    SRC_COLOR,
    ONE_MINUS_SRC_COLOR,
    DST_COLOR,
    ONE_MINUS_DST_COLOR,
    SRC_ALPHA,
    ONE_MINUS_SRC_ALPHA,
    DST_ALPHA,
    ONE_MINUS_DST_ALPHA,
    CONSTANT_COLOR,
    ONE_MINUS_CONSTANT_COLOR,
    CONSTANT_ALPHA,
    ONE_MINUS_CONSTANT_ALPHA,
    SRC_ALPHA_SATURATE,
    SRC1_COLOR,
    ONE_MINUS_SRC1_COLOR,
    SRC1_ALPHA,
    ONE_MINUS_SRC1_ALPHA,
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

enum class GPUImageLayout {
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

union ClearValue
{
    float color[4];
    struct ClearDepthStencil
    {
        float depth;
        uint32_t stencil;
    } depth_stencil;
};

struct GPUImageDesc {
    u32 width = 0;
    u32 height = 0;
    u32 depth = 0;
    u32 arraySize = 1;
    u32 mipLevels = 1;
    GPUImageType type = GPUImageType::TYPE_MAX;
    DataFormat format =  DataFormat::R8G8B8A8_UNORM;
    GPUImageSamples samples = GPUImageSamples::SAMPLES_1;
    u32 usageBits  = 0;
    bool isSwapChainImage = false;
};

struct GPUImage {
    GPUImageDesc desc;
    void* internal;
};

struct Shader {
    ShaderStage stage = ShaderStage::MAX_ENUM;
    void* internal;
};

struct PipelineColorBlendState {
    bool enable_logic_op = false;
    LogicOperation logic_op = LogicOperation::CLEAR;

    struct Attachment {
        bool enable_blend = false;
        BlendFactor srcColorBlendFactor = BlendFactor::ZERO;
        BlendFactor dstColorBlendFactor = BlendFactor::ZERO;
        BlendOperation colorBlendOp = BlendOperation::ADD;
        BlendFactor srcAlphaBlendFactor = BlendFactor::ZERO;
        BlendFactor dstAlphaBlendFactor = BlendFactor::ZERO;
        BlendOperation alphaBlendOp = BlendOperation::ADD;
        bool writeR = true;
        bool writeG = true;
        bool writeB = true;
        bool writeA = true;
    };

    static PipelineColorBlendState create_disabled(int p_attachments = 1) {
        PipelineColorBlendState bs;
        for (int i = 0; i < p_attachments; i++) {
            bs.attachments.push_back(Attachment());
        }
        return bs;
    }

    static PipelineColorBlendState create_blend(int p_attachments = 1) {
        PipelineColorBlendState bs;
        for (int i = 0; i < p_attachments; i++) {
            Attachment ba;
            ba.enable_blend = true;
            ba.srcColorBlendFactor = BlendFactor::SRC_ALPHA;
            ba.dstColorBlendFactor = BlendFactor::ONE_MINUS_SRC_ALPHA;
            ba.srcAlphaBlendFactor = BlendFactor::SRC_ALPHA;
            ba.dstColorBlendFactor = BlendFactor::ONE_MINUS_SRC_ALPHA;

            bs.attachments.push_back(ba);
        }
        return bs;
    }

    std::vector<Attachment> attachments; // One per render target texture.
};

struct PipeLineVertexAttribute {
    uint32_t location = 0; // Shader location.
    uint32_t offset = 0;
    DataFormat format = DataFormat::MAX_ENUM;
    uint32_t stride = 0;
};

struct RasterizationState {
    CullMode cullMode = CullMode::NONE;
    bool isWireFrame = false;
    bool frontFaceCCW = false;
    bool enableDepthClamp = false;
    bool enableMultisample = false;
    bool enableAntialiasedLine = false;
    uint32_t SampleCount = 0;
    float lineWidth = 1.f;
};

struct PipelineDepthStencilState {
    bool enableDepthTest = false;
    bool enableDepthWrite = false;
    CompareOperation depthCompareOp = CompareOperation::MAX_ENUM;
    bool enableDepthRange = false;
    float depthRangeMin = 0;
    float depthRangeMax = 0;
    bool enableStencil = false;
};

struct PipeLineDesc {
    Shader* vertShader = nullptr;
    Shader* fragShader = nullptr;
    Shader* compShader = nullptr;
    PipelineColorBlendState blendState = {};
    RasterizationState rasterState = {};
    PipelineDepthStencilState depthStencilState = {};
    PrimitiveTopology  primitiveTopo = PrimitiveTopology::MAX_ENUM;
    std::vector<PipeLineVertexAttribute> vertexAttribs;
    // For dynamic rendering
    std::vector<DataFormat> colorAttachmentFormats;
    DataFormat depthAttachmentFormat = DataFormat::MAX_ENUM; 
};

struct PipeLine {
    PipeLineDesc desc = {};
    void* internal = nullptr;
};

struct Vertex3D {
	glm::vec3 position;
	float uv_x;
	glm::vec3 normal;
	float uv_y;
	glm::vec4 color;

    bool operator==(const Vertex3D &other) const {
        return position == other.position && color == other.color && normal == other.normal &&
                uv_x == other.uv_x && uv_y == other.uv_y;
    }
};

// Buffer
struct GPUBufferDesc {

};

struct GPUBuffer {

};

struct RenderPassAttachment{
    enum class Type
    {
        RENDERTARGET,
        DEPTH_STENCIL,
    } type = Type::RENDERTARGET;

    enum class LoadOp
    {
        LOAD,
        CLEAR,
        DONTCARE,
    } loadOp = LoadOp::LOAD;

    enum class StoreOp
    {
        STORE,
        DONTCARE,
    } storeOp = StoreOp::STORE;

    GPUImage* image = nullptr;
    ClearValue clearValue = {};

    // GPUImageLayout layoutBeforeRenderPass = GPUImageLayout::UNDEFINED;
    // GPUImageLayout layoutInRenderPass = GPUImageLayout::UNDEFINED;
    // GPUImageLayout layoutAfterRenderPass = GPUImageLayout::UNDEFINED;
};

struct RenderPass {
    constexpr static u32 MAX_COLOR_ATTHACHEMNT_NUM = 8;
    enum class AttachmentLoadOp
    {
        LOAD,
        CLEAR,
        DONTCARE,
    };

    enum class AttachmentStoreOp
    {
        STORE,
        DONTCARE,
    };

    // Color images
    GPUImage* colorAttachments[MAX_COLOR_ATTHACHEMNT_NUM];
    ClearValue clearColors[MAX_COLOR_ATTHACHEMNT_NUM] = {};
    AttachmentLoadOp colorAttatchemtsLoadOp[MAX_COLOR_ATTHACHEMNT_NUM];
    AttachmentStoreOp colorAttatchemtsStoreOp[MAX_COLOR_ATTHACHEMNT_NUM];
    u32 numColorAttachments = 0;

    // Resolve image. The declaration of color 
    // attachments and resolve attachments must match
    // to deduce source color image and the destination resolve image.
    GPUImage* resolveAttatchments[MAX_COLOR_ATTHACHEMNT_NUM];
    u32 numResolveAttachments = 0;

    // Depth images
    GPUImage* depthAttatchment = nullptr;
    AttachmentLoadOp depthAttachmentLoadOp = AttachmentLoadOp::LOAD;
    AttachmentStoreOp depthAttachmentStoreOp = AttachmentStoreOp::STORE;
    ClearValue ClearDepthStencil = {1.f, 0.f};
};

struct PipelineMemoryBarrier
{
    u32 srcStageBits = 0;
    u32 dstStageBits = 0;
    u32 srcMemoryAccessBits = 0;
    u32 dstMemoryAccessBits = 0;
};

// we don't actually use this...
struct PipelineBufferBarrier
{
    GPUBuffer* buffer = nullptr;
    u32 srcStageBits = 0;
    u32 dstStageBits = 0;
    u32 srcMemoryAccessBits = 0;
    u32 dstMemoryAccessBits = 0;
};

// In addition to memory barrier, we need to convert the layout(a state) of a image
struct PipelineImageBarrier
{
    GPUImage* image = nullptr;
    u32 srcStageBits = 0;
    u32 dstStageBits = 0;
    u32 srcMemoryAccessBits = 0;
    u32 dstMemoryAccessBits = 0;
    GPUImageLayout layoutBefore;
    GPUImageLayout layoutAfter;
    u32 baseMipLevel = UINT32_MAX;
    u32 baseArrayLayer = UINT32_MAX;
};
