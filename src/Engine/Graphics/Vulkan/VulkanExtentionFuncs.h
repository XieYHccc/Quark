#pragma once
#include "pch.h"

struct VulkanExtentionFuncs {

    static void Init();

    static PFN_vkQueueSubmit2KHR pVkQueueSubmit2KHR;
    static PFN_vkCmdBlitImage2KHR pVkCmdBlitImage2KHR;
    static PFN_vkCmdPipelineBarrier2KHR pVkCmdPipelineBarrier2KHR;
    static PFN_vkCmdBeginRenderingKHR pVkCmdBeginRenderingKHR;
    static PFN_vkCmdEndRenderingKHR pVkCmdEndRenderingKHR;
};