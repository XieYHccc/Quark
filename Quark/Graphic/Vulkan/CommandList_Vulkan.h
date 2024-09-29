#pragma once
#include "Quark/Graphic/Vulkan/Common_Vulkan.h"
#include "Quark/Graphic/CommandList.h"
#include "Quark/Graphic/RenderPassInfo.h"
#include "Quark/Graphic/Vulkan/PipeLine_Vulkan.h"

class UI_Vulkan;

namespace quark::graphic {

enum class CommandListState {
    READY_FOR_RECORDING,
    IN_RECORDING,
    IN_RENDERPASS,
    READY_FOR_SUBMIT,
};

class CommandList_Vulkan : public CommandList {
public:
    CommandListState state = CommandListState::READY_FOR_RECORDING;

    CommandList_Vulkan(Device_Vulkan* device, QueueType type_);
    ~CommandList_Vulkan();

    void PushConstant(const void* data, uint32_t offset, uint32_t size) override;
    void BindPipeLine(const PipeLine& pipeline) override;
    void BindUniformBuffer(u32 set, u32 binding, const Buffer& buffer, u64 offset, u64 size) override;
    void BindStorageBuffer(u32 set, u32 binding, const Buffer& buffer, u64 offset, u64 size) override;
    void BindImage(u32 set, u32 binding, const Image& image, ImageLayout layout) override;
    void BindVertexBuffer(u32 binding, const Buffer& buffer, u64 offset) override;
    void BindIndexBuffer(const Buffer& buffer, u64 offset, const IndexBufferFormat format) override;
    void BindSampler(u32 set, u32 binding, const Sampler& sampler) override;
    
    void Draw(u32 vertex_count, u32 instance_count, u32 first_vertex, u32 first_instance) override;
    void DrawIndexed(u32 index_count, u32 instance_count, u32 first_index, u32 vertex_offset, u32 first_instance) override;

    void SetViewPort(const Viewport& viewport) override;
    void SetScissor(const Scissor& scissor) override;

    void PipeLineBarriers(const PipelineMemoryBarrier* memoryBarriers, u32 memoryBarriersCount, const PipelineImageBarrier* imageBarriers, u32 iamgeBarriersCount, const PipelineBufferBarrier* bufferBarriers, u32 bufferBarriersCount) override;
    
    void BeginRenderPass(const RenderPassInfo2& renderPassInfo, const FrameBufferInfo& frameBufferInfo) override;
    // void BeginRenderPass(const RenderPassInfo& info) override;
    void EndRenderPass() override;

    ///////////////////////// Vulkan specific /////////////////////////

    void ResetAndBeginCmdBuffer();
    bool IsWaitingForSwapChainImage() const { return m_WaitForSwapchainImage; }

    const VkCommandBuffer GetHandle() const { return m_CmdBuffer; }
    const VkSemaphore GetCmdCompleteSemaphore() const { return m_CmdCompleteSemaphore; }
    uint32_t GetSwapChainWaitStages() const { return m_SwapChainWaitStages; }

    
private:
    void FlushDescriptorSet(u32 set);
    void FlushRenderState();
    void RebindDescriptorSet(u32 set);  // Rebind if only the buffer offset changed
    void ResetBindingState();

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
    };


    Device_Vulkan* m_GraphicDevice;

    VkSemaphore m_CmdCompleteSemaphore = VK_NULL_HANDLE;
    VkCommandBuffer m_CmdBuffer = VK_NULL_HANDLE;
    VkCommandPool m_CmdPool = VK_NULL_HANDLE;

    std::vector<VkMemoryBarrier2> m_MemoryBarriers;
    std::vector<VkImageMemoryBarrier2> m_ImageBarriers;
    std::vector<VkBufferMemoryBarrier2> m_BufferBarriers;
    bool m_WaitForSwapchainImage = false;
    uint32_t m_SwapChainWaitStages = 0;

    // Rendering state 
    //const RenderPassInfo* m_CurrentRenderPassInfo = nullptr;
    const PipeLine_Vulkan* m_CurrentPipeline = nullptr;
    RenderPassInfo2 m_CurrentRenderPassInfo2 = {};
    VkDescriptorSet m_CurrentSets[DESCRIPTOR_SET_MAX_NUM] = {};
    VkViewport m_Viewport = {};
    VkRect2D m_Scissor = {};
    BindingState m_BindingState = {};

    uint32_t m_DirtySetMask = 0;
    uint32_t m_DirtySetDynamicMask = 0;
    uint32_t m_DirtyVertexBufferMask = 0;
};

CONVERT_TO_VULKAN_INTERNAL_FUNC(CommandList)
}