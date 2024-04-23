#include "Graphics/Vulkan/VulkanExtentionFuncs.h"
#include "Graphics/Vulkan/RendererVulkan.h"

PFN_vkQueueSubmit2KHR VulkanExtentionFuncs::pVkQueueSubmit2KHR;
PFN_vkCmdBlitImage2KHR VulkanExtentionFuncs::pVkCmdBlitImage2KHR = nullptr;
PFN_vkCmdPipelineBarrier2KHR VulkanExtentionFuncs::pVkCmdPipelineBarrier2KHR = nullptr;
PFN_vkCmdBeginRenderingKHR VulkanExtentionFuncs::pVkCmdBeginRenderingKHR = nullptr;
PFN_vkCmdEndRenderingKHR VulkanExtentionFuncs::pVkCmdEndRenderingKHR = nullptr;

void VulkanExtentionFuncs::Init()
{
    pVkQueueSubmit2KHR = (PFN_vkQueueSubmit2KHR)vkGetDeviceProcAddr(
        RendererVulkan::GetInstance()->GetVkDevice(), "vkQueueSubmit2KHR");
    pVkCmdBlitImage2KHR = (PFN_vkCmdBlitImage2KHR)vkGetDeviceProcAddr(
        RendererVulkan::GetInstance()->GetVkDevice(), "vkCmdBlitImage2KHR");
    pVkCmdPipelineBarrier2KHR = (PFN_vkCmdPipelineBarrier2KHR)vkGetDeviceProcAddr(
        RendererVulkan::GetInstance()->GetVkDevice(), "vkCmdPipelineBarrier2KHR");
    pVkCmdBeginRenderingKHR = (PFN_vkCmdBeginRenderingKHR)vkGetDeviceProcAddr(
        RendererVulkan::GetInstance()->GetVkDevice(), "vkCmdBeginRenderingKHR");
    pVkCmdEndRenderingKHR = (PFN_vkCmdEndRenderingKHR)vkGetDeviceProcAddr(
        RendererVulkan::GetInstance()->GetVkDevice(), "vkCmdEndRenderingKHR");
}