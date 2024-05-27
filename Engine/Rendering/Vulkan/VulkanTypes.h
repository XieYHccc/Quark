#pragma once
#include <vulkan/vulkan.h>
#include <vulkan/vk_enum_string_helper.h>
#include <vk_mem_alloc.h>
#include <glm/glm.hpp>

#define VK_CHECK(x)                                                             \
    do {                                                                         \
        VkResult err = x;                                                        \
        if (err) {                                                               \
            CORE_LOGC("Detected Vulkan error: {}", string_VkResult(err)) \
            abort();                                                             \
        }                                                                        \
    } while (0);


// Render pass
enum RenderPassState_Vulkan {
    READY,
    RECORDING,
    IN_RENDER_PASS,
    RECORDING_ENDED,
    SUBMITTED,
    NOT_ALLOCATED
};

struct RenderPass_Vulkan {
    VkRenderPass handle;
    glm::vec4 region;
    glm::vec4 clearColor;

    f32 depth;
    u32 stencil;

    RenderPassState_Vulkan state;
};

// Command buffer
enum CommandBufferState_Vulkan {
    COMMAND_BUFFER_STATE_READY,
    COMMAND_BUFFER_STATE_RECORDING,
    COMMAND_BUFFER_STATE_IN_RENDER_PASS,
    COMMAND_BUFFER_STATE_RECORDING_ENDED,
    COMMAND_BUFFER_STATE_SUBMITTED,
    COMMAND_BUFFER_STATE_NOT_ALLOCATED
};

struct CommandBuffer_Vulkan {
    VkCommandBuffer handle = VK_NULL_HANDLE;

    // Command buffer state.
    CommandBufferState_Vulkan state = COMMAND_BUFFER_STATE_NOT_ALLOCATED;
};