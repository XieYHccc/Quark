#include "pch.h"
#include "Rendering/Vulkan/VulkanCommandBuffer.h"
#include "Rendering/Vulkan/RenderDeviceDriver_Vulkan.h"

void vulkan_command_buffer_allocate(
    RenderDeviceDriver_Vulkan* driver,
    VkCommandPool pool,
    bool is_primary,
    CommandBuffer_Vulkan* out_command_buffer) {

    CORE_DEBUG_ASSERT(out_command_buffer->handle == VK_NULL_HANDLE)

    VkCommandBufferAllocateInfo allocate_info = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    allocate_info.commandPool = pool;
    allocate_info.level = is_primary ? VK_COMMAND_BUFFER_LEVEL_PRIMARY : VK_COMMAND_BUFFER_LEVEL_SECONDARY;
    allocate_info.commandBufferCount = 1;
    allocate_info.pNext = 0;

    out_command_buffer->state = COMMAND_BUFFER_STATE_NOT_ALLOCATED;
    VK_CHECK(vkAllocateCommandBuffers(
        driver->device->logicalDevice,
        &allocate_info,
        &out_command_buffer->handle));
    out_command_buffer->state = COMMAND_BUFFER_STATE_READY;
}

void vulkan_command_buffer_free(
    RenderDeviceDriver_Vulkan* driver,
    VkCommandPool pool,
    CommandBuffer_Vulkan* command_buffer) {
    vkFreeCommandBuffers(
        driver->device->logicalDevice,
        pool,
        1,
        &command_buffer->handle);

    command_buffer->handle = VK_NULL_HANDLE;
    command_buffer->state = COMMAND_BUFFER_STATE_NOT_ALLOCATED;
}

void vulkan_command_buffer_begin(
    CommandBuffer_Vulkan* command_buffer,
    bool is_single_use,
    bool is_renderpass_continue,
    bool is_simultaneous_use) {
    
    VkCommandBufferBeginInfo begin_info = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    begin_info.flags = 0;
    if (is_single_use) {
        begin_info.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    }
    if (is_renderpass_continue) {
        begin_info.flags |= VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;
    }
    if (is_simultaneous_use) {
        begin_info.flags |= VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
    }

    VK_CHECK(vkBeginCommandBuffer(command_buffer->handle, &begin_info));
    command_buffer->state = COMMAND_BUFFER_STATE_RECORDING;
}

void vulkan_command_buffer_end(CommandBuffer_Vulkan* command_buffer) {
    VK_CHECK(vkEndCommandBuffer(command_buffer->handle));
    command_buffer->state = COMMAND_BUFFER_STATE_RECORDING_ENDED;
}

void vulkan_command_buffer_update_submitted(CommandBuffer_Vulkan* command_buffer) {
    command_buffer->state = COMMAND_BUFFER_STATE_SUBMITTED;
}

void vulkan_command_buffer_reset(CommandBuffer_Vulkan* command_buffer) {
    command_buffer->state = COMMAND_BUFFER_STATE_READY;
}

void vulkan_command_buffer_allocate_and_begin_single_use(
    RenderDeviceDriver_Vulkan* driver,
    VkCommandPool pool,
    CommandBuffer_Vulkan* out_command_buffer) {
    vulkan_command_buffer_allocate(driver, pool, true, out_command_buffer);
    vulkan_command_buffer_begin(out_command_buffer, true, false, false);
}

void vulkan_command_buffer_end_single_use(
    RenderDeviceDriver_Vulkan* driver,
    VkCommandPool pool,
    CommandBuffer_Vulkan* command_buffer,
    VkQueue queue) {

    // End the command buffer.
    vulkan_command_buffer_end(command_buffer);

    // Submit the queue
    VkSubmitInfo submit_info = {VK_STRUCTURE_TYPE_SUBMIT_INFO};
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &command_buffer->handle;
    VK_CHECK(vkQueueSubmit(queue, 1, &submit_info, 0));

    // Wait for it to finish
    VK_CHECK(vkQueueWaitIdle(queue));

    // Free the command buffer.
    vulkan_command_buffer_free(driver, pool, command_buffer);
 }