#include "Quark/qkpch.h"
#include "Quark/RHI/Vulkan/PipeLine_Vulkan.h"
#include "Quark/RHI/Vulkan/Shader_Vulkan.h"
#include "Quark/RHI/Vulkan/Device_Vulkan.h"

namespace quark::rhi {

constexpr VkPrimitiveTopology _ConvertTopologyType(TopologyType topo)
{
    switch (topo) {
    case TopologyType::TRANGLE_LIST:
        return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    case TopologyType::LINE_LIST:
        return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;        
    case TopologyType::POINT_LIST:
        return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
    default:
        return VK_PRIMITIVE_TOPOLOGY_MAX_ENUM;
    }
}

constexpr VkPolygonMode _ConvertPolygonMode(PolygonMode mode)
{
    switch (mode) {
    case PolygonMode::Line:
        return VK_POLYGON_MODE_LINE;
    case PolygonMode::Fill:
        return VK_POLYGON_MODE_FILL;
    default:
        QK_CORE_VERIFY("Polygon mode not handeled yet!")
        return VK_POLYGON_MODE_MAX_ENUM;
    }
}

constexpr VkCullModeFlagBits _ConverCullMode(CullMode mode)
{
    switch (mode) {
    case CullMode::BACK:
        return VK_CULL_MODE_BACK_BIT;
        break;
    case CullMode::FRONT:
        return VK_CULL_MODE_FRONT_BIT;
        break;
    case CullMode::NONE:
        return VK_CULL_MODE_NONE;
        break;
    default:
        QK_CORE_VERIFY(false)
        return VK_CULL_MODE_NONE;
    }
}

constexpr VkBlendFactor _ConvertBlendFactor(BlendFactor factor)
{
    switch (factor)
    {
    case BlendFactor::ZERO:
        return VK_BLEND_FACTOR_ZERO;
    case BlendFactor::ONE:
        return VK_BLEND_FACTOR_ONE;
    case BlendFactor::SRC_COLOR:
        return VK_BLEND_FACTOR_SRC_COLOR;
    case BlendFactor::ONE_MINUS_SRC_COLOR:
        return VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
    case BlendFactor::SRC_ALPHA:
        return VK_BLEND_FACTOR_SRC_ALPHA;
    case BlendFactor::ONE_MINUS_SRC_ALPHA:
        return VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    case BlendFactor::DST_ALPHA:
        return VK_BLEND_FACTOR_DST_ALPHA;
    case BlendFactor::ONE_MINUS_DST_ALPHA:
        return VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
    case BlendFactor::DST_COLOR:
        return VK_BLEND_FACTOR_DST_COLOR;
    case BlendFactor::ONE_MINUS_DST_COLOR:
        return VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR;
    case BlendFactor::SRC_ALPHA_SATURATE:
        return VK_BLEND_FACTOR_SRC_ALPHA_SATURATE;
    case BlendFactor::CONSTANT_COLOR:
        return VK_BLEND_FACTOR_CONSTANT_COLOR;
    case BlendFactor::ONE_MINUS_CONSTANT_COLOR:
        return VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR;
    case BlendFactor::SRC1_COLOR:
        return VK_BLEND_FACTOR_SRC1_COLOR;
    case BlendFactor::ONE_MINUS_SRC1_COLOR:
        return VK_BLEND_FACTOR_ONE_MINUS_SRC1_COLOR;
    case BlendFactor::SRC1_ALPHA:
        return VK_BLEND_FACTOR_SRC1_ALPHA;
    case BlendFactor::ONE_MINUS_SRC1_ALPHA:
        return VK_BLEND_FACTOR_ONE_MINUS_SRC1_ALPHA;
    default:
        return VK_BLEND_FACTOR_ZERO;
    }
}

constexpr VkBlendOp _ConvertBlendOp(BlendOperation value)
{
    switch (value)
    {
    case BlendOperation::ADD:
        return VK_BLEND_OP_ADD;
    case BlendOperation::SUBTRACT:
        return VK_BLEND_OP_SUBTRACT;
    case BlendOperation::REVERSE_SUBTRACT:
        return VK_BLEND_OP_REVERSE_SUBTRACT;
    case BlendOperation::MINIMUM:
        return VK_BLEND_OP_MIN;
    case BlendOperation::MAXIMUM:
        return VK_BLEND_OP_MAX;
    default:
        return VK_BLEND_OP_ADD;
    }
}

PipeLineLayout::PipeLineLayout(Device_Vulkan* _device, const ShaderResourceLayout _combinedLayout)
    : device(_device), combinedLayout(_combinedLayout)
{
    QK_CORE_VERIFY(this->device != nullptr)

    // Descriptor set layouts
    std::vector<VkDescriptorSetLayout> vk_descriptorset_layouts;
    for (uint32_t set = 0; set < DESCRIPTOR_SET_MAX_NUM; set++) 
    {
        if ((combinedLayout.descriptorSetLayoutMask & 1u << set) == 0)
            continue;

        setAllocators[set] = this->device->Request_DescriptorSetAllocator(combinedLayout.descriptorSetLayouts[set]);
        vk_descriptorset_layouts.push_back(setAllocators[set]->GetLayout());
    }

    // Pipeline layout 
    VkPipelineLayoutCreateInfo pipeline_layout_create_info = {};
    pipeline_layout_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_create_info.pSetLayouts = vk_descriptorset_layouts.data();
    pipeline_layout_create_info.setLayoutCount = (uint32_t)vk_descriptorset_layouts.size();
    if (combinedLayout.pushConstant.size > 0) 
    {
        pipeline_layout_create_info.pushConstantRangeCount = 1;
        pipeline_layout_create_info.pPushConstantRanges = &combinedLayout.pushConstant;
    }
    else 
    {
        pipeline_layout_create_info.pushConstantRangeCount = 0;
        pipeline_layout_create_info.pPushConstantRanges = nullptr;
    }

    VK_CHECK(vkCreatePipelineLayout(this->device->vkDevice, &pipeline_layout_create_info, nullptr, &handle))

    // Create descriptor set update template
    for (size_t set = 0; set < DESCRIPTOR_SET_MAX_NUM; ++set) 
    {
        if ((combinedLayout.descriptorSetLayoutMask & (1u << set)) == 0)
            continue;

        VkDescriptorUpdateTemplateEntry update_entries[SET_BINDINGS_MAX_NUM];
        uint32_t update_count = 0;

        auto& set_layout = combinedLayout.descriptorSetLayouts[set];
        for (auto& binding : set_layout.bindings) {
            QK_CORE_ASSERT(binding.binding < SET_BINDINGS_MAX_NUM)

            auto& entry = update_entries[update_count++];
            entry.dstBinding = binding.binding;
            entry.descriptorType = binding.descriptorType;
            entry.descriptorCount = binding.descriptorCount;
            entry.dstArrayElement = 0;
            entry.stride = sizeof(DescriptorBinding);
            switch (binding.descriptorType) {
            case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
            case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
            case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
                entry.offset = offsetof(DescriptorBinding, buffer) + sizeof(DescriptorBinding) * binding.binding;
                break;
            case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
            case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
            case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
            case VK_DESCRIPTOR_TYPE_SAMPLER:
                entry.offset = offsetof(DescriptorBinding, image) + sizeof(DescriptorBinding) * binding.binding;
                break;
            default:
                QK_CORE_VERIFY(0, "Descriptor type not handled yet.")
            }
        }

        VkDescriptorUpdateTemplateCreateInfo info = { VK_STRUCTURE_TYPE_DESCRIPTOR_UPDATE_TEMPLATE_CREATE_INFO };
        info.pipelineLayout = handle;
        info.descriptorSetLayout = setAllocators[set]->GetLayout();
        info.templateType = VK_DESCRIPTOR_UPDATE_TEMPLATE_TYPE_DESCRIPTOR_SET;
        info.set = (uint32_t)set;
        info.descriptorUpdateEntryCount = update_count;
        info.pDescriptorUpdateEntries = update_entries;
        info.pipelineBindPoint = (set_layout.set_stage_mask & VK_SHADER_STAGE_COMPUTE_BIT)? VK_PIPELINE_BIND_POINT_COMPUTE : VK_PIPELINE_BIND_POINT_GRAPHICS;

        if (vkCreateDescriptorUpdateTemplate(device->vkDevice, &info, nullptr, &updateTemplate[set]) != VK_SUCCESS)
            QK_CORE_VERIFY(0, "Failed to create descriptor update template")
    }

}

PipeLineLayout::~PipeLineLayout()
{
    for (size_t set = 0; set < DESCRIPTOR_SET_MAX_NUM; ++set) 
    {
        if (combinedLayout.descriptorSetLayoutMask & (1u << set)) 
            vkDestroyDescriptorUpdateTemplate(device->vkDevice, updateTemplate[set], nullptr);
    }

    vkDestroyPipelineLayout(device->vkDevice, handle, nullptr);

    QK_CORE_LOGT_TAG("RHI", "Pipeline layout destroyed");
}

PipeLine_Vulkan::PipeLine_Vulkan(Device_Vulkan* device, const GraphicPipeLineDesc& desc)
    :PipeLine(PipeLineBindingPoint::GRAPHIC), m_device(device), m_graphicDesc(desc)
{
    // Combine shader's resource bindings and create pipeline layout
    {
        ShaderResourceLayout combinedLayout;
        auto insert_shader = [&](Ref<Shader> shader)
        {
            Shader_Vulkan& internal_shader = ToInternal(shader.get());
            const ShaderResourceLayout& shaderResourceLayout = internal_shader.GetResourceLayout();

            // loop each set of bindings
            for (size_t i = 0; i < DESCRIPTOR_SET_MAX_NUM; ++i) 
            {
                if ((shaderResourceLayout.descriptorSetLayoutMask & 1u << i) == 0) 
                    continue;

                combinedLayout.descriptorSetLayoutMask |= 1u << i;
                const DescriptorSetLayout& srcSetLayout = shaderResourceLayout.descriptorSetLayouts[i];
                DescriptorSetLayout& dstSetLayout = combinedLayout.descriptorSetLayouts[i];
               
                for (size_t j = 0; j < srcSetLayout.bindings.size(); ++j) 
                {
                    const VkDescriptorSetLayoutBinding& x = shaderResourceLayout.descriptorSetLayouts[i].bindings[j];
                    VkDescriptorSetLayoutBinding& y = combinedLayout.descriptorSetLayouts[i].vk_bindings[j];

                    y.binding = x.binding;
                    y.descriptorCount = x.descriptorCount;
                    y.descriptorType = x.descriptorType;
                    y.stageFlags |= x.stageFlags;

                    dstSetLayout.set_stage_mask |= x.stageFlags;

                    //TODO: This shouldn't belong here, move to shader reflection code.
                    switch (x.descriptorType) {
                    case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
                    case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
                        dstSetLayout.uniform_buffer_mask |= 1u << x.binding;
                        break;
                    case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
                        dstSetLayout.storage_buffer_mask |= 1u << x.binding;
                        break;
                    case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
                        dstSetLayout.sampled_image_mask |= 1u << x.binding;
                        break;
                    case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
                        dstSetLayout.separate_image_mask |= 1u << x.binding;
                        break;
                    case VK_DESCRIPTOR_TYPE_SAMPLER:
                        dstSetLayout.sampler_mask |= 1u << x.binding;
                        break;
                    default:
                        QK_CORE_VERIFY("Descriptor type not handled!")
                        break;
                    }
                    
                    bool found = false;
                    for (auto& z : combinedLayout.descriptorSetLayouts[i].bindings)
                    {
                        if (x.binding == z.binding) 
                        {
							// If the asserts fire, it means there are overlapping bindings between shader stages
							// This is not supported now for performance reasons (less binding management)!
							// (Overlaps between s/b/t bind points are not a problem because those are shifted by the compiler)
                            QK_CORE_VERIFY(x.descriptorCount == z.descriptorCount)
                            QK_CORE_VERIFY(x.descriptorType == z.descriptorType)
                            found = true;
                            z.stageFlags |= x.stageFlags;
                            break;
                        }
                    }

                    if (!found) 
                    {
                        dstSetLayout.bindings.push_back(x);
                        // tmp_layout.imageViewTypes[i].push_back(internal_shader.bindingViews_[i][j]);
                    }
                }
            }

            // push constant
            const VkPushConstantRange& shaderPushConstant = shaderResourceLayout.pushConstant;
            VkPushConstantRange& combinedPushConstant = combinedLayout.pushConstant;
            if (shaderPushConstant.size > 0) 
            {
                combinedPushConstant.offset = std::min(shaderPushConstant.offset, combinedPushConstant.offset);
                combinedPushConstant.size = std::max(shaderPushConstant.size, combinedPushConstant.size);
                combinedPushConstant.stageFlags |= shaderPushConstant.stageFlags;
            }

        };

        insert_shader(desc.vertShader);
        insert_shader(desc.fragShader);

        // Create Pipeline Layout
        m_layout = m_device->Request_PipeLineLayout(combinedLayout);
    }

    // Shaderstage Info
    auto& vert_shader_internal = ToInternal(desc.vertShader.get());
    auto& frag_shader_internal = ToInternal(desc.fragShader.get());
    VkPipelineShaderStageCreateInfo shader_stage_info[2] = { vert_shader_internal.GetStageInfo(), frag_shader_internal.GetStageInfo()};

    // Input Assembly
    VkPipelineInputAssemblyStateCreateInfo input_assembly = {};
    input_assembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    input_assembly.topology = _ConvertTopologyType(desc.topologyType);
    input_assembly.primitiveRestartEnable = VK_FALSE;

    // Rasterization.
    VkPipelineRasterizationStateCreateInfo rasterization_state_create_info = { VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };
    rasterization_state_create_info.depthClampEnable = desc.rasterState.enableDepthClamp;
    rasterization_state_create_info.rasterizerDiscardEnable = VK_FALSE;
    rasterization_state_create_info.polygonMode = _ConvertPolygonMode(desc.rasterState.polygonMode);
    rasterization_state_create_info.frontFace = (desc.rasterState.frontFaceType == FrontFaceType::COUNTER_CLOCKWISE ? VK_FRONT_FACE_COUNTER_CLOCKWISE : VK_FRONT_FACE_CLOCKWISE);
    rasterization_state_create_info.lineWidth = desc.rasterState.lineWidth;
    rasterization_state_create_info.cullMode = _ConverCullMode(desc.rasterState.cullMode);
    rasterization_state_create_info.depthBiasEnable = VK_FALSE; // TODO: Make depth bias configurable
    rasterization_state_create_info.depthBiasConstantFactor = 0;
    rasterization_state_create_info.depthBiasClamp = 0;
    rasterization_state_create_info.depthBiasSlopeFactor = 0;

    // View Port : using dynamic view port 
    VkPipelineViewportStateCreateInfo viewport_state = {};
    viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport_state.viewportCount = 1;
    viewport_state.pViewports = nullptr;
    viewport_state.scissorCount = 1;
    viewport_state.pScissors = nullptr;

    // Depth stencil
    VkPipelineDepthStencilStateCreateInfo depth_stencil_state_create_info = {};
    depth_stencil_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depth_stencil_state_create_info.depthTestEnable = desc.depthStencilState.enableDepthTest;
    depth_stencil_state_create_info.depthWriteEnable = desc.depthStencilState.enableDepthWrite;
    depth_stencil_state_create_info.depthCompareOp = ConvertCompareOp(desc.depthStencilState.depthCompareOp);
    depth_stencil_state_create_info.stencilTestEnable = VK_FALSE; // TODO: Support stencil test and depth bounds test
    depth_stencil_state_create_info.depthBoundsTestEnable = VK_FALSE;

    // Vertex Input
    VkPipelineVertexInputStateCreateInfo vertex_input_create_info = {};
    vertex_input_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertex_input_create_info.pNext = nullptr;

    const rhi::VertexInputLayout& vertexInputLayout = desc.vertexInputLayout;
    std::vector<VkVertexInputBindingDescription> bindings(vertexInputLayout.vertexBindInfos.size());
    std::vector<VkVertexInputAttributeDescription> attributes(vertexInputLayout.vertexAttribInfos.size());
    if (vertexInputLayout.isValid()) 
    {
        for (size_t i = 0; i < bindings.size(); i++)
        {
            bindings[i].binding = vertexInputLayout.vertexBindInfos[i].binding;
            bindings[i].stride = vertexInputLayout.vertexBindInfos[i].stride;

            if (vertexInputLayout.vertexBindInfos[i].inputRate == VertexInputLayout::VertexBindInfo::INPUT_RATE_VERTEX)
                bindings[i].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
            if (vertexInputLayout.vertexBindInfos[i].inputRate == VertexInputLayout::VertexBindInfo::INPUT_RATE_INSTANCE)
                bindings[i].inputRate = VK_VERTEX_INPUT_RATE_INSTANCE;
        }

        for (size_t i = 0; i < attributes.size(); i++)
        {
            attributes[i].binding = vertexInputLayout.vertexAttribInfos[i].binding;
            attributes[i].location = vertexInputLayout.vertexAttribInfos[i].location;
            attributes[i].offset = vertexInputLayout.vertexAttribInfos[i].offset;
           
            switch (vertexInputLayout.vertexAttribInfos[i].format)
            {
            case VertexInputLayout::VertexAttribInfo::ATTRIB_FORMAT_VEC2:
                attributes[i].format = VK_FORMAT_R32G32_SFLOAT;
				break;
            case VertexInputLayout::VertexAttribInfo::ATTRIB_FORMAT_VEC3:
                attributes[i].format = VK_FORMAT_R32G32B32_SFLOAT;
                break;
            case VertexInputLayout::VertexAttribInfo::ATTRIB_FORMAT_VEC4:
                attributes[i].format = VK_FORMAT_R32G32B32A32_SFLOAT;
                break;
            case VertexInputLayout::VertexAttribInfo::ATTRIB_FORMAT_UVEC4:
                attributes[i].format = VK_FORMAT_R32G32B32A32_UINT;
				break;
            default:
                QK_CORE_VERIFY(0)
                break;
            }
        }
        
        vertex_input_create_info.vertexBindingDescriptionCount = (uint32_t)bindings.size();
        vertex_input_create_info.vertexAttributeDescriptionCount = (uint32_t)attributes.size();
        vertex_input_create_info.pVertexBindingDescriptions = bindings.data();
        vertex_input_create_info.pVertexAttributeDescriptions = attributes.data();
    }
    else 
    {  
        // looks like buffer device address is used for vertex input.
        vertex_input_create_info.pVertexAttributeDescriptions = nullptr;
        vertex_input_create_info.pVertexBindingDescriptions = nullptr;
        vertex_input_create_info.vertexAttributeDescriptionCount = 0;
        vertex_input_create_info.vertexBindingDescriptionCount = 0;
    }

    // Dynamic state
    const uint32_t dynamic_state_count = 2;
    VkDynamicState dynamic_states[dynamic_state_count] =
    {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };

    VkPipelineDynamicStateCreateInfo dynamic_state_create_info = {};
    dynamic_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamic_state_create_info.dynamicStateCount = dynamic_state_count;
    dynamic_state_create_info.pDynamicStates = dynamic_states;

    // TODO: Support tessellation

    // Multisampling
    VkPipelineMultisampleStateCreateInfo multisample_state_create_info = { VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };
    multisample_state_create_info.sampleShadingEnable = VK_FALSE;
    multisample_state_create_info.rasterizationSamples = (VkSampleCountFlagBits)desc.renderPassInfo.sampleCount;
    multisample_state_create_info.minSampleShading = 1.0f;
    multisample_state_create_info.pSampleMask = 0;
    multisample_state_create_info.alphaToCoverageEnable = VK_FALSE;
    multisample_state_create_info.alphaToOneEnable = VK_FALSE;

    // Blend state
    uint32_t numBlendAttachments = 0;
    VkPipelineColorBlendAttachmentState colorBlendAttachments[8] = {};
    for (size_t i = 0; i < desc.renderPassInfo.numColorAttachments; ++i)
    {
        size_t attachmentIndex = 0;
        if (desc.blendState.enable_independent_blend)
            attachmentIndex = i;

        auto& att_desc = desc.blendState.attachments[attachmentIndex];
        VkPipelineColorBlendAttachmentState& att = colorBlendAttachments[numBlendAttachments++];

        att.blendEnable = att_desc.enable_blend ? VK_TRUE : VK_FALSE;
        att.colorWriteMask = 0;
        if (att_desc.colorWriteMask & util::ecast(ColorWriteFlagBits::ENABLE_RED))
            att.colorWriteMask |= VK_COLOR_COMPONENT_R_BIT;
        if (att_desc.colorWriteMask & util::ecast(ColorWriteFlagBits::ENABLE_GREEN))
            att.colorWriteMask |= VK_COLOR_COMPONENT_G_BIT;
        if (att_desc.colorWriteMask & util::ecast(ColorWriteFlagBits::ENABLE_BLUE))
            att.colorWriteMask |= VK_COLOR_COMPONENT_B_BIT;
        if (att_desc.colorWriteMask & util::ecast(ColorWriteFlagBits::ENABLE_ALPHA))
            att.colorWriteMask |= VK_COLOR_COMPONENT_A_BIT;

        att.srcColorBlendFactor = _ConvertBlendFactor(att_desc.srcColorBlendFactor);
        att.dstColorBlendFactor = _ConvertBlendFactor(att_desc.dstColorBlendFactor);
        att.colorBlendOp = _ConvertBlendOp(att_desc.colorBlendOp);
        att.srcAlphaBlendFactor = _ConvertBlendFactor(att_desc.srcAlphaBlendFactor);
        att.dstAlphaBlendFactor = _ConvertBlendFactor(att_desc.srcAlphaBlendFactor);
        att.alphaBlendOp = _ConvertBlendOp(att_desc.alphaBlendOp);
    }

    VkPipelineColorBlendStateCreateInfo color_blend_state_create_info = {};
    color_blend_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    color_blend_state_create_info.logicOpEnable = VK_FALSE; // TODO: Support logic operation
    color_blend_state_create_info.logicOp = VK_LOGIC_OP_COPY;
    color_blend_state_create_info.attachmentCount = numBlendAttachments;
    color_blend_state_create_info.pAttachments = colorBlendAttachments;
    color_blend_state_create_info.blendConstants[0] = 1.0f;
    color_blend_state_create_info.blendConstants[1] = 1.0f;
    color_blend_state_create_info.blendConstants[2] = 1.0f;
    color_blend_state_create_info.blendConstants[3] = 1.0f;

    // Rendering info : we are using dynamic rendering instead of renderpass and framebuffer
    const auto& renderPassInfo2 = desc.renderPassInfo;
    std::vector<VkFormat> vk_color_attachment_format;
    vk_color_attachment_format.resize(renderPassInfo2.numColorAttachments);
    for (size_t i = 0; i < renderPassInfo2.numColorAttachments; i++)
        vk_color_attachment_format[i] = ConvertDataFormat(renderPassInfo2.colorAttachmentFormats[i]);

    VkPipelineRenderingCreateInfo renderingInfo = {};
    renderingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
    renderingInfo.colorAttachmentCount = (uint32_t)vk_color_attachment_format.size();
    renderingInfo.pColorAttachmentFormats = vk_color_attachment_format.data();
    if (IsFormatSupportDepth(renderPassInfo2.depthAttachmentFormat))
        renderingInfo.depthAttachmentFormat = ConvertDataFormat(renderPassInfo2.depthAttachmentFormat);

    // Finally, fill pipeline create info
    VkGraphicsPipelineCreateInfo pipeline_create_info = {};
    pipeline_create_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipeline_create_info.pStages = shader_stage_info;
    pipeline_create_info.stageCount = 2;
    pipeline_create_info.pVertexInputState = &vertex_input_create_info;
    pipeline_create_info.pInputAssemblyState = &input_assembly;
    pipeline_create_info.pViewportState = &viewport_state;
    pipeline_create_info.pRasterizationState = &rasterization_state_create_info;
    pipeline_create_info.pMultisampleState = &multisample_state_create_info;
    pipeline_create_info.pDepthStencilState = &depth_stencil_state_create_info;
    pipeline_create_info.pColorBlendState = &color_blend_state_create_info;
    pipeline_create_info.pDynamicState = &dynamic_state_create_info;
    pipeline_create_info.layout = m_layout->handle;
    pipeline_create_info.pNext = &renderingInfo;
    pipeline_create_info.renderPass = nullptr;
    VK_CHECK(vkCreateGraphicsPipelines(m_device->vkDevice, nullptr, 1, &pipeline_create_info, nullptr, &m_handle))
}

PipeLine_Vulkan::~PipeLine_Vulkan()
{
    if (m_handle != VK_NULL_HANDLE) 
    {
        auto& frame = m_device->GetCurrentFrame();
        frame.garbagePipelines.push_back(m_handle);
    }

    QK_CORE_LOGT_TAG("RHI", "Vulkan pipeline destroyed");
}

}