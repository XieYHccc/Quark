#pragma once
#include "Graphic/Vulkan/Common_Vulkan.h"
#include "Graphic/CommandList.h"
#include "Graphic/RenderPassInfo.h"
#include "Graphic/Vulkan/PipeLine_Vulkan.h"

class UI_Vulkan;

namespace graphic {

enum class CommandListState {
    READY_FOR_RECORDING,
    IN_RECORDING,
    IN_RENDERPASS,
    READY_FOR_SUBMIT,
};

class CommandList_Vulkan : public CommandList {
    friend class Device_Vulkan;
    friend class ::UI_Vulkan;
public:
    CommandList_Vulkan(Device_Vulkan* device, QueueType type_);
    ~CommandList_Vulkan();

    void BindPipeLine(const PipeLine& pipeline) override;
    void BindPushConstant(const void* data, size_t offset, size_t size) override;
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
    void BeginRenderPass(const RenderPassInfo& info) override;
    void EndRenderPass() override;

private:
    void Flush_DescriptorSet(u32 set);
    void Flush_RenderState();
    void Rebind_DescriptorSet(u32 set);
    void ResetAndBeginCmdBuffer();
    
private:
    Device_Vulkan* device_;
    VkSemaphore cmdCompleteSemaphore_;
    VkCommandBuffer cmdBuffer_ = VK_NULL_HANDLE;
    VkCommandPool cmdPool_ = VK_NULL_HANDLE;
    std::vector<VkMemoryBarrier2> memoryBarriers_;
    std::vector<VkImageMemoryBarrier2> imageBarriers_;
    std::vector<VkBufferMemoryBarrier2> bufferBarriers_;
    bool waitForSwapchainImage_ = false;
    u32 swapChainWaitStages_ = 0;
    CommandListState state_ = CommandListState::READY_FOR_RECORDING;

    // Rendering state 
    const RenderPassInfo* currentRenderPassInfo_ = nullptr;
    const PipeLine_Vulkan* currentPipeLine_ = nullptr;
    VkDescriptorSet currentSets[DESCRIPTOR_SET_MAX_NUM] = {};
    VkViewport viewport_ = {};
    VkRect2D scissor_ = {};

    struct BindingState {
        DescriptorBinding descriptorBindings[DESCRIPTOR_SET_MAX_NUM][SET_BINDINGS_MAX_NUM] = {};
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

    } bindingState_;


    u32 dirty_SetBits_ = 0;
    u32 ditry_SetDynamicBits_ = 0;
    u32 dirty_VertexBufferBits_ = 0;
};

CONVERT_TO_VULKAN_INTERNAL(CommandList)
}