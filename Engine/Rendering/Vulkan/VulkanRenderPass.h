#pragma once
#include "Rendering/Vulkan/VulkanTypes.h"
#include "Rendering/Vulkan/VulkanRenderPass.h"
#include "Rendering/Vulkan/VulkanCommandBuffer.h"

class RenderDeviceDriver_Vulkan;

void VulkanRenderPassCreate(
    RenderDeviceDriver_Vulkan* driver,
    RenderPass_Vulkan* out_renderpass,
    const glm::vec4& region,
    const glm::vec4& ckearColor,
    f32 depth,
    u32 stencil);

void VulkanRenderPassFree(RenderDeviceDriver_Vulkan* driver, RenderPass_Vulkan* renderpass);

void VulkanRenderPassBegin(
    CommandBuffer_Vulkan* command_buffer, 
    RenderPass_Vulkan* renderpass,
    VkFramebuffer frame_buffer);

void VulkanRenderPassEnd(CommandBuffer_Vulkan* command_buffer, RenderPass_Vulkan* renderpass);