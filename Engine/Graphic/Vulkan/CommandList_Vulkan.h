#pragma once
#include "pch.h"
#include "Graphic/Vulkan/Common_Vulkan.h"
#include "Graphic/CommandList.h"
#include "Graphic/RenderPassInfo.h"
#include "Graphic/Vulkan/Shader_Vulkan.h"

namespace graphic {
struct ShaderResouceBinding {
    union{
        VkDescriptorBufferInfo buffer;
        VkDescriptorImageInfo image;
    };
};

struct ShaderResourceBinder
{
    ShaderResouceBinding bindings[Shader::SHADER_RESOURCE_SET_MAX_NUM][Shader::SET_BINDINGS_MAX_NUM];
    uint64_t cookies[Shader::SHADER_RESOURCE_SET_MAX_NUM][Shader::SET_BINDINGS_MAX_NUM];
};

enum CommandListDirtyBits
{
	COMMAN_LIST_DIRTY_STATIC_STATE_BIT = 1 << 0,
	COMMAN_LIST_DIRTY_PIPELINE_BIT = 1 << 1,
	COMMAN_LIST_DIRTY_VIEWPORT_BIT = 1 << 2,
	COMMAN_LIST_DIRTY_SCISSOR_BIT = 1 << 3,
	COMMAN_LIST_DIRTY_STATIC_VERTEX_BIT = 1 << 6,
	COMMAN_LIST_DIRTY_PUSH_CONSTANTS_BIT = 1 << 7,
	COMMAN_LIST_DIRTY_DYNAMIC_BITS = COMMAN_LIST_DIRTY_VIEWPORT_BIT | COMMAN_LIST_DIRTY_SCISSOR_BIT
};

class CommandList_Vulkan : public CommandList {
    friend class Device_Vulkan;
public:
    CommandList_Vulkan(Device_Vulkan* device, QueueType type_);
    ~CommandList_Vulkan();

    void BindPushConstant(const void* data, size_t offset, size_t size) override;
    void BindUniformBuffer(u32 set, u32 binding, const Buffer& buffer, uint64_t offset = 0, uint64_t size = 0) override;
    void BindPipeLine(PipeLineType type, const PipeLine& pipeline) override;
    void SetViewPort(const Viewport& viewport) override;
    void SetScissor(const Scissor& scissor) override;
    void PipeLineBarriers(const PipelineMemoryBarrier* memoryBarriers, u32 memoryBarriersCount, const PipelineImageBarrier* imageBarriers, u32 iamgeBarriersCount, const PipelineBufferBarrier* bufferBarriers, u32 bufferBarriersCount) override;
    void BeginRenderPass(const RenderPassInfo& info) override;
    void EndRenderPass() override;
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

    // Rendering structures
    VkViewport viewport_;
    VkRect2D scissor_;
    const PipeLine_Vulkan* currentPipeLine_;
    ShaderResouceBinding bindings_[Shader::SHADER_RESOURCE_SET_MAX_NUM][Shader::SET_BINDINGS_MAX_NUM];
    uint32_t dirtyBits_;
    uint32_t dirtySetBits_;
};

CONVERT_TO_VULKAN_INTERNAL(CommandList)
}