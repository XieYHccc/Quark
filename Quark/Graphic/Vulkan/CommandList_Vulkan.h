#pragma once
#include "Quark/Graphic/Vulkan/Common_Vulkan.h"
#include "Quark/Graphic/CommandList.h"
#include "Quark/Graphic/RenderPassInfo.h"
#include "Quark/Graphic/Vulkan/PipeLine_Vulkan.h"

class UI_Vulkan;

namespace quark::graphic {

enum CommandListDirtyFlagBits
{
    COMMAND_LIST_DIRTY_PUSH_CONSTANTS_BIT = 1 << 0,
};

enum class CommandListState {
    READY_FOR_RECORDING,
    IN_RECORDING,
    IN_RENDERPASS,
    READY_FOR_SUBMIT,
};

class CommandList_Vulkan final : public CommandList {
public:
    CommandListState state = CommandListState::READY_FOR_RECORDING;

    CommandList_Vulkan(Device_Vulkan* device, QueueType type_);
    ~CommandList_Vulkan();

    void PushConstant(const void* data, uint32_t offset, uint32_t size) override final;
    void BindPipeLine(const PipeLine& pipeline) override final;
    void BindUniformBuffer(uint32_t set, uint32_t binding, const Buffer& buffer, uint64_t offset, uint64_t size) override final;
    void BindStorageBuffer(uint32_t set, uint32_t binding, const Buffer& buffer, u64 offset, u64 size) override final;
    void BindImage(uint32_t set, uint32_t binding, const Image& image, ImageLayout layout) override final;
    void BindVertexBuffer(uint32_t binding, const Buffer& buffer, u64 offset) override final;
    void BindIndexBuffer(const Buffer& buffer, u64 offset, const IndexBufferFormat format) override final;
    void BindSampler(uint32_t set, uint32_t binding, const Sampler& sampler) override final;
    
    void CopyImageToBuffer(const Buffer& buffer, const Image& image, uint64_t buffer_offset, const Offset3D& offset, const Extent3D& extent, uint32_t row_pitch, uint32_t slice_pitch, const ImageSubresourceRange& subresouce) override final;
    
    void Draw(uint32_t vertex_count, uint32_t instance_count, uint32_t first_vertex, uint32_t first_instance) override final;
    void DrawIndexed(uint32_t index_count, uint32_t instance_count, uint32_t first_index, uint32_t vertex_offset, uint32_t first_instance) override final;

    void SetViewPort(const Viewport& viewport) override final;
    void SetScissor(const Scissor& scissor) override final;

    void PipeLineBarriers(const PipelineMemoryBarrier* memoryBarriers, uint32_t memoryBarriersCount, const PipelineImageBarrier* imageBarriers, uint32_t iamgeBarriersCount, const PipelineBufferBarrier* bufferBarriers, uint32_t bufferBarriersCount) override final;
    
    void BeginRenderPass(const RenderPassInfo2& renderPassInfo, const FrameBufferInfo& frameBufferInfo) override final;
    // void BeginRenderPass(const RenderPassInfo& info) override;
    void EndRenderPass() override final;

    const RenderPassInfo2& GetCurrentRenderPassInfo() const override final;
    const PipeLine* GetCurrentGraphicsPipeline() const override final;

    ///////////////////////// Vulkan specific /////////////////////////

    void ResetAndBeginCmdBuffer();
    bool IsWaitingForSwapChainImage() const { return m_waitForSwapchainImage; }

    const VkCommandBuffer GetHandle() const { return m_cmdBuffer; }
    const VkSemaphore GetCmdCompleteSemaphore() const { return m_cmdCompleteSemaphore; }
    uint32_t GetSwapChainWaitStages() const { return m_swapChainWaitStages; }
    
private:
    void FlushDescriptorSet(uint32_t set);
    void FlushRenderState();
    void RebindDescriptorSet(uint32_t set);  // Rebind if only the buffer offset changed
    void ResetBindingState();

    void _SetDirtyFlags(CommandListDirtyFlagBits flags) { m_dirtyMask |= flags; }

    CommandListDirtyFlagBits _GetAndClearDirtyFlags(CommandListDirtyFlagBits flags);
private:
    struct BindingState
    {
        struct VertexBufferBindingState
        {
            VkBuffer buffers[VERTEX_BUFFER_MAX_NUM] = {};
            VkDeviceSize offsets[VERTEX_BUFFER_MAX_NUM] = {};
        } vertexBufferBindingState;

        struct IndexBufferBindingState
        {
            VkBuffer buffer = VK_NULL_HANDLE;
            VkDeviceSize offset = 0;
            IndexBufferFormat format = IndexBufferFormat::UINT32;
        } indexBufferBindingState;

        DescriptorBinding descriptorBindings[DESCRIPTOR_SET_MAX_NUM][SET_BINDINGS_MAX_NUM] = {};

        uint8_t pushConstantData[PUSH_CONSTANT_DATA_SIZE];
    };


    Device_Vulkan* m_device;

    VkSemaphore m_cmdCompleteSemaphore = VK_NULL_HANDLE;
    VkCommandBuffer m_cmdBuffer = VK_NULL_HANDLE;
    VkCommandPool m_cmdPool = VK_NULL_HANDLE;

    std::vector<VkMemoryBarrier2> m_memoryBarriers;
    std::vector<VkImageMemoryBarrier2> m_imageBarriers;
    std::vector<VkBufferMemoryBarrier2> m_bufferBarriers;
    bool m_waitForSwapchainImage = false;
    uint32_t m_swapChainWaitStages = 0;

    // Rendering state 
    const PipeLine_Vulkan* m_currentPipeline = nullptr;
    RenderPassInfo2 m_currentRenderPassInfo = {};
    VkDescriptorSet m_currentSets[DESCRIPTOR_SET_MAX_NUM] = {};
    VkViewport m_viewport = {};
    VkRect2D m_scissor = {};
    BindingState m_bindingState = {};

    uint32_t m_dirtySetMask = 0;
    uint32_t m_dirtySetRebindMask = 0;
    uint32_t m_dirtyVertexBufferMask = 0;
    uint32_t m_dirtyMask = ~0u;
    
};

CONVERT_TO_VULKAN_INTERNAL_FUNC(CommandList)
}