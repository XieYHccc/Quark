#pragma once
#include <vulkan/vulkan.h>
#include <vulkan/vk_enum_string_helper.h>
#include <vk_mem_alloc.h>
#include <glm/glm.hpp>

#include "Quark/RHI/Common.h"

namespace quark::rhi {

constexpr uint32_t VULKAN_MAX_UBO_SIZE = 64 * 1024;

#define VK_CHECK(x)                                                                        \
    do {                                                                                   \
        VkResult err = x;                                                                  \
        if (err) {                                                                         \
            QK_CORE_LOGF_TAG("RHI","Detected Vulkan error: {}", string_VkResult(err)); \
            abort();                                                                       \
        }                                                                                  \
    } while (0);

// Forward declaration
#define VULKAN_INTERNAL_CLASS(x_) \
    class x_##_Vulkan; \
    x_##_Vulkan& ToInternal(x_*); \
    const x_##_Vulkan& ToInternal(const x_*); \

VULKAN_INTERNAL_CLASS(Device)
VULKAN_INTERNAL_CLASS(Buffer)
VULKAN_INTERNAL_CLASS(Image)
VULKAN_INTERNAL_CLASS(CommandList)
VULKAN_INTERNAL_CLASS(Shader)
VULKAN_INTERNAL_CLASS(PipeLine)
VULKAN_INTERNAL_CLASS(Sampler)

#undef VULKAN_INTERNAL_CLASS

#define CONVERT_TO_VULKAN_INTERNAL_FUNC(x_) \
    inline x_##_Vulkan& ToInternal(x_* ptr) { return *static_cast<x_##_Vulkan*>(ptr);} \
    inline const x_##_Vulkan& ToInternal(const x_* ptr) {return *static_cast<const x_##_Vulkan*>(ptr);}\

constexpr VkFormat ConvertDataFormat(DataFormat value)
{
    switch(value)
    {
    case DataFormat::UNDEFINED:
        return VK_FORMAT_UNDEFINED;
    case DataFormat::B8G8R8A8_UNORM:
        return VK_FORMAT_B8G8R8A8_UNORM;
    case DataFormat::R8G8B8A8_UNORM:
        return VK_FORMAT_R8G8B8A8_UNORM;
    case DataFormat::D32_SFLOAT:
        return VK_FORMAT_D32_SFLOAT;
    case DataFormat::D32_SFLOAT_S8_UINT:
        return VK_FORMAT_D32_SFLOAT_S8_UINT;
    case DataFormat::D24_UNORM_S8_UINT:
        return VK_FORMAT_D24_UNORM_S8_UINT;
    case DataFormat::R32G32B32_SFLOAT:
        return VK_FORMAT_R32G32B32_SFLOAT;
    case DataFormat::BC7_UNORM_BLOCK:
        return VK_FORMAT_BC7_UNORM_BLOCK;
    case DataFormat::BC3_UNORM_BLOCK:
        return VK_FORMAT_BC3_UNORM_BLOCK;
    case DataFormat::R16G16B16A16_SFLOAT:
        return VK_FORMAT_R16G16B16A16_SFLOAT;
    case DataFormat::R64_UINT:
        return VK_FORMAT_R64_UINT;
    case DataFormat::R32_UINT:
        return VK_FORMAT_R32_UINT;    
    case DataFormat::R32_SINT:
        return VK_FORMAT_R32_SINT;
    case DataFormat::R32G32_UINT:
        return VK_FORMAT_R32G32_UINT;
    default:
        QK_CORE_VERIFY(0, "Data format not handled yet!")
        return VK_FORMAT_UNDEFINED;
    }
}

constexpr VkCompareOp ConvertCompareOp(CompareOperation op)
{
    switch (op) 
    {
    case CompareOperation::NEVER:
        return VK_COMPARE_OP_NEVER;
    case CompareOperation::LESS:
        return VK_COMPARE_OP_LESS;
    case CompareOperation::EQUAL:
        return VK_COMPARE_OP_EQUAL;
    case CompareOperation::LESS_OR_EQUAL:
        return VK_COMPARE_OP_LESS_OR_EQUAL;
    case CompareOperation::GREATER:
        return VK_COMPARE_OP_GREATER;
    case CompareOperation::NOT_EQUAL:
        return VK_COMPARE_OP_NOT_EQUAL;
    case CompareOperation::GREATER_OR_EQUAL:
        return VK_COMPARE_OP_GREATER_OR_EQUAL;
    case CompareOperation::ALWAYS:
        return VK_COMPARE_OP_ALWAYS;
    default:
    case CompareOperation::MAX_ENUM:
        return VK_COMPARE_OP_MAX_ENUM;
    }
}

}