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
    uint32_t srcStageBits = 0;
    uint32_t dstStageBits = 0;
    uint32_t srcMemoryAccessBits = 0;
    uint32_t dstMemoryAccessBits = 0;
};

// we don't actually use this...
struct PipelineBufferBarrier
{
    Buffer* buffer;
    uint32_t srcStageBits = 0;
    uint32_t dstStageBits = 0;
    uint32_t srcMemoryAccessBits = 0;
    uint32_t dstMemoryAccessBits = 0;
};

// In addition to memory barrier, we need to convert the layout(a state) of a image
struct PipelineImageBarrier
{
    Image* image;
    uint32_t srcStageBits = 0;
    uint32_t dstStageBits = 0;
    uint32_t srcMemoryAccessBits = 0;
    uint32_t dstMemoryAccessBits = 0;
    ImageLayout layoutBefore;
    ImageLayout layoutAfter;
    uint32_t baseMipLevel = UINT32_MAX;
    uint32_t baseArrayLayer = UINT32_MAX;
};

class CommandList : public GpuResource {
public:
    CommandList(QueueType type) : m_QueueType(type) {};
    virtual ~CommandList() = default;

    virtual void PushConstant(const void* data, uint32_t offset, uint32_t size) = 0;
    virtual void BindUniformBuffer(uint32_t set, uint32_t binding, const Buffer& buffer, uint64_t offset, uint64_t size) = 0;   
    virtual void BindStorageBuffer(uint32_t set, uint32_t binding, const Buffer& buffer, uint64_t offset, uint64_t size) = 0;
    virtual void BindImage(uint32_t set, uint32_t binding, const Image& image, ImageLayout layout) = 0;
    virtual void BindPipeLine(const PipeLine& pipeline) = 0;
    virtual void BindVertexBuffer(uint32_t binding, const Buffer& buffer, uint64_t offset) = 0;
    virtual void BindIndexBuffer(const Buffer& buffer, uint64_t offset, const IndexBufferFormat format) = 0;
    virtual void BindSampler(uint32_t set, uint32_t binding, const Sampler& sampler) = 0;
    
    virtual void CopyImageToBuffer(const Buffer& buffer, const Image& image, uint64_t buffer_offset, Offset3D& offset, Extent3D& extent, uint32_t row_pitch, uint32_t slice_pitch, ImageSubresourceRange& subresouce) = 0;

    virtual void DrawIndexed(uint32_t index_count, uint32_t instance_count, uint32_t first_index, uint32_t vertex_offset, uint32_t first_instance) = 0;
    virtual void Draw(uint32_t vertex_count, uint32_t instance_count, uint32_t first_vertex, uint32_t first_instance) = 0;
    
    virtual void SetViewPort(const Viewport& viewport) = 0;
    virtual void SetScissor(const Scissor& scissor) = 0;

    virtual void PipeLineBarriers(const PipelineMemoryBarrier* memoryBarriers, uint32_t memoryBarriersCount, const PipelineImageBarrier* iamgeBarriers, uint32_t iamgeBarriersCount, const PipelineBufferBarrier* bufferBarriers, uint32_t bufferBarriersCount) = 0;
    
    // RenderPassInfo2 here is used to check the formats compatibilies between renderpass and framebuffer
    virtual void BeginRenderPass(const RenderPassInfo2& renderPassInfo, const FrameBufferInfo& frameBufferInfo) = 0;
    // virtual void BeginRenderPass(const RenderPassInfo& info) = 0;
    virtual void EndRenderPass() = 0;
    
    QueueType GetQueueType() const { return m_QueueType; }
    GpuResourceType GetGpuResourceType() const override { return GpuResourceType::COMMAND_LIST; }

protected:
    QueueType m_QueueType;
};

}