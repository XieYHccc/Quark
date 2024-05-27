#pragma once
#include "Rendering/Vulkan/VulkanTypes.h"

class RenderDeviceDriver_Vulkan;

void vulkan_command_buffer_allocate(
    RenderDeviceDriver_Vulkan* driver,
    VkCommandPool pool,
    bool is_primary,
    CommandBuffer_Vulkan* out_command_buffer);

void vulkan_command_buffer_free(
    RenderDeviceDriver_Vulkan* driver,
    VkCommandPool pool,
    CommandBuffer_Vulkan* command_buffer);

void vulkan_command_buffer_begin(
    CommandBuffer_Vulkan* command_buffer,
    bool is_single_use,
    bool is_renderpass_continue,
    bool is_simultaneous_use);

void vulkan_command_buffer_end(CommandBuffer_Vulkan* command_buffer);

void vulkan_command_buffer_update_submitted(CommandBuffer_Vulkan* command_buffer);

void vulkan_command_buffer_reset(CommandBuffer_Vulkan* command_buffer);

/**
 * Allocates and begins recording to out_command_buffer.
 */
void vulkan_command_buffer_allocate_and_begin_single_use(
    RenderDeviceDriver_Vulkan* driver,
    VkCommandPool pool,
    CommandBuffer_Vulkan* out_command_buffer);

/**
 * Ends recording, submits to and waits for queue operation and frees the provided command buffer.
 */
void vulkan_command_buffer_end_single_use(
    RenderDeviceDriver_Vulkan* driver,
    VkCommandPool pool,
    CommandBuffer_Vulkan* command_buffer,
    VkQueue queue);