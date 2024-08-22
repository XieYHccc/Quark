#pragma once
#include "Quark/Graphic/Buffer.h"
#include "Quark/Graphic/Common.h"
#include "Quark/Graphic/Image.h"
#include "Quark/Graphic/PipeLine.h"

namespace quark::graphic {
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
    Buffer* buffer;
    u32 srcStageBits = 0;
    u32 dstStageBits = 0;
    u32 srcMemoryAccessBits = 0;
    u32 dstMemoryAccessBits = 0;
};

// In addition to memory barrier, we need to convert the layout(a state) of a image
struct PipelineImageBarrier
{
    Image* image;
    u32 srcStageBits = 0;
    u32 dstStageBits = 0;
    u32 srcMemoryAccessBits = 0;
    u32 dstMemoryAccessBits = 0;
    ImageLayout layoutBefore;
    ImageLayout layoutAfter;
    u32 baseMipLevel = UINT32_MAX;
    u32 baseArrayLayer = UINT32_MAX;
};

class CommandList : public GpuResource {
public:
    CommandList(QueueType type) : m_QueueType(type) {};
    virtual ~CommandList() = default;

    virtual void BindPushConstant(const void* data, size_t offset, size_t size) = 0;
    virtual void BindUniformBuffer(u32 set, u32 binding, const Buffer& buffer, u64 offset, u64 size) = 0;   
    virtual void BindStorageBuffer(u32 set, u32 binding, const Buffer& buffer, u64 offset, u64 size) = 0;
    virtual void BindImage(u32 set, u32 binding, const Image& image, ImageLayout layout) = 0;
    virtual void BindPipeLine(const PipeLine& pipeline) = 0;
    virtual void BindVertexBuffer(u32 binding, const Buffer& buffer, u64 offset) = 0;
    virtual void BindIndexBuffer(const Buffer& buffer, u64 offset, const IndexBufferFormat format) = 0;
    virtual void BindSampler(u32 set, u32 binding, const Sampler& sampler) = 0;
    
    virtual void DrawIndexed(u32 index_count, u32 instance_count, u32 first_index, u32 vertex_offset, u32 first_instance) = 0;
    virtual void Draw(u32 vertex_count, u32 instance_count, u32 first_vertex, u32 first_instance) = 0;
    
    virtual void SetViewPort(const Viewport& viewport) = 0;
    virtual void SetScissor(const Scissor& scissor) = 0;

    virtual void PipeLineBarriers(const PipelineMemoryBarrier* memoryBarriers, u32 memoryBarriersCount, const PipelineImageBarrier* iamgeBarriers, u32 iamgeBarriersCount, const PipelineBufferBarrier* bufferBarriers, u32 bufferBarriersCount) = 0;
    virtual void BeginRenderPass(const RenderPassInfo& info) = 0;
    virtual void EndRenderPass() = 0;
    
    QueueType GetQueueType() const { return m_QueueType; }
    GpuResourceType GetGpuResourceType() const override { return GpuResourceType::COMMAND_LIST; }

protected:
    QueueType m_QueueType;
};

}