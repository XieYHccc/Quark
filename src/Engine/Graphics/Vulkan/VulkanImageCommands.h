#pragma once
#include "pch.h"

namespace vkutil {

void TransitionImage(VkDevice device, VkCommandBuffer cmd, VkImage image, VkImageLayout currentLayout, VkImageLayout newLayout);

void CopyImageToImage(VkDevice device, VkCommandBuffer cmd, VkImage source, VkImage destination, VkExtent2D srcSize, VkExtent2D dstSize);

}