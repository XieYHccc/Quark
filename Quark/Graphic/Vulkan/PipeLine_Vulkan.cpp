#include "Quark/qkpch.h"
#include "Quark/Graphic/Vulkan/PipeLine_Vulkan.h"
#include "Quark/Graphic/Vulkan/Shader_Vulkan.h"
#include "Quark/Graphic/Vulkan/Device_Vulkan.h"

namespace quark::graphic {

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
        CORE_ASSERT("Polygon mode not handeled yet!")
        break;
    }
}

constexpr VkCullModeFlagBits _ConverCullMode(CullMode mode)
{
    switch (mode) {
    case CullMode::BACK:
        return VK_CULL_MODE_BACK_BIT;
    case CullMode::FRONT:
        return VK_CULL_MODE_FRONT_BIT;
    case CullMode::NONE:
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
    CORE_DEBUG_ASSERT(this->device != nullptr)

    // Descriptor set layouts
    std::vector<VkDescriptorSetLayout> vk_descriptorset_layouts;
    for (size_t set = 0; set < DESCRIPTOR_SET_MAX_NUM; set++) 
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
    pipeline_layout_create_info.setLayoutCount = vk_descriptorset_layouts.size();
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
            CORE_DEBUG_ASSERT(binding.binding < SET_BINDINGS_MAX_NUM)

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
                CORE_ASSERT_MSG(0, "Descriptor type not handled yet.")
            }
        }

        VkDescriptorUpdateTemplateCreateInfo info = { VK_STRUCTURE_TYPE_DESCRIPTOR_UPDATE_TEMPLATE_CREATE_INFO };
        info.pipelineLayout = handle;
        info.descriptorSetLayout = setAllocators[set]->GetLayout();
        info.templateType = VK_DESCRIPTOR_UPDATE_TEMPLATE_TYPE_DESCRIPTOR_SET;
        info.set = set;
        info.descriptorUpdateEntryCount = update_count;
        info.pDescriptorUpdateEntries = update_entries;
        info.pipelineBindPoint = (set_layout.set_stage_mask & VK_SHADER_STAGE_COMPUTE_BIT)? VK_PIPELINE_BIND_POINT_COMPUTE : VK_PIPELINE_BIND_POINT_GRAPHICS;

        if (vkCreateDescriptorUpdateTemplate(device->vkDevice, &info, nullptr, &updateTemplate[set]) != VK_SUCCESS)
            CORE_LOGE("Failed to create descriptor update template")
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

    CORE_LOGD("Pipeline layout destroyed")
}

PipeLine_Vulkan::PipeLine_Vulkan(Device_Vulkan* device, const GraphicPipeLineDesc& desc)
    :PipeLine(PipeLineBindingPoint::GRAPHIC), m_Device(device), m_CompatableRenderPassInfo(desc.renderPassInfo)
{
    CORE_DEBUG_ASSERT(m_Device)
    CORE_DEBUG_ASSERT(desc.vertShader != nullptr && desc.fragShader != nullptr)

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
                        CORE_DEBUG_ASSERT("Descriptor type not handled!")
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
                            CORE_ASSERT(x.descriptorCount == z.descriptorCount)
                            CORE_ASSERT(x.descriptorType == z.descriptorType)
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
        m_Layout = m_Device->Request_PipeLineLayout(combinedLayout);
    }

    // Shaderstage Info
    auto& vert_shader_internal = ToInternal(desc.vertShader.get());
    auto& frag_shader_internal = ToInternal(desc.fragShader.get());
    VkPipelineShaderStageCreateInfo shader_stage_info[2] = { vert_shader_internal.GetStageInfo(), frag_shader_internal.GetStageInfo()};

    // Vertex Input
    VkPipelineVertexInputStateCreateInfo vertex_input_create_info = {};
    vertex_input_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertex_input_create_info.pNext = nullptr;
    std::vector<VkVertexInputBindingDescription> bindings(desc.vertexBindInfos.size());
    std::vector<VkVertexInputAttributeDescription> attributes(desc.vertexAttribInfos.size());
    if (!desc.vertexAttribInfos.empty() && !desc.vertexBindInfos.empty()) {
        for (size_t i = 0; i < bindings.size(); i++)
        {
            bindings[i].binding = desc.vertexBindInfos[i].binding;
            bindings[i].stride = desc.vertexBindInfos[i].stride;

            if (desc.vertexBindInfos[i].inputRate == VertexBindInfo::INPUT_RATE_VERTEX)
                bindings[i].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
            if (desc.vertexBindInfos[i].inputRate == VertexBindInfo::INPUT_RATE_INSTANCE)
                bindings[i].inputRate = VK_VERTEX_INPUT_RATE_INSTANCE;
        }

        for (size_t i = 0; i < attributes.size(); i++)
        {
            attributes[i].binding = desc.vertexAttribInfos[i].binding;
            attributes[i].location = desc.vertexAttribInfos[i].location;
            attributes[i].offset = desc.vertexAttribInfos[i].offset;
            
            if(desc.vertexAttribInfos[i].format == VertexAttribInfo::ATTRIB_FORMAT_VEC2)
                attributes[i].format = VK_FORMAT_R32G32_SFLOAT;
            if (desc.vertexAttribInfos[i].format == VertexAttribInfo::ATTRIB_FORMAT_VEC3)
                attributes[i].format = VK_FORMAT_R32G32B32_SFLOAT;
            if (desc.vertexAttribInfos[i].format == VertexAttribInfo::ATTRIB_FORMAT_VEC4)
                attributes[i].format = VK_FORMAT_R32G32B32A32_SFLOAT;
        }
        
        vertex_input_create_info.vertexBindingDescriptionCount = (uint32_t)bindings.size();
        vertex_input_create_info.vertexAttributeDescriptionCount = (uint32_t)attributes.size();
        vertex_input_create_info.pVertexBindingDescriptions = bindings.data();
        vertex_input_create_info.pVertexAttributeDescriptions = attributes.data();
    }
    else {  // looks like buffer device address is used for vertex input.
        vertex_input_create_info.pVertexAttributeDescriptions = nullptr;
        vertex_input_create_info.pVertexBindingDescriptions = nullptr;
        vertex_input_create_info.vertexAttributeDescriptionCount = 0;
        vertex_input_create_info.vertexBindingDescriptionCount = 0;
    }

    // Input Assembly
	VkPipelineInputAssemblyStateCreateInfo input_assembly = {};
    input_assembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	input_assembly.topology = _ConvertTopologyType(desc.topologyType);
	input_assembly.primitiveRestartEnable = VK_FALSE;

    // View Port : using dynamic view port 
    VkPipelineViewportStateCreateInfo viewport_state = {};
    viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport_state.viewportCount = 1;
    viewport_state.scissorCount = 1;

    // TODO: Support tessellation

    // Rasterization.
    VkPipelineRasterizationStateCreateInfo rasterization_state_create_info = {VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
	rasterization_state_create_info.depthClampEnable = desc.rasterState.enableDepthClamp;
	rasterization_state_create_info.rasterizerDiscardEnable = VK_FALSE;
	rasterization_state_create_info.polygonMode = _ConvertPolygonMode(desc.rasterState.polygonMode);
	rasterization_state_create_info.frontFace = (desc.rasterState.frontFaceType == FrontFaceType::COUNTER_CLOCKWISE ? VK_FRONT_FACE_COUNTER_CLOCKWISE : VK_FRONT_FACE_CLOCKWISE );
    rasterization_state_create_info.lineWidth = desc.rasterState.lineWidth;
    rasterization_state_create_info.cullMode = _ConverCullMode(desc.rasterState.cullMode);
    // TODO: Make depth bias configurable
	rasterization_state_create_info.depthBiasEnable = VK_FALSE;
	rasterization_state_create_info.depthBiasConstantFactor = 0;
	rasterization_state_create_info.depthBiasClamp = 0;
	rasterization_state_create_info.depthBiasSlopeFactor = 0;

    // Multisampling
    VkPipelineMultisampleStateCreateInfo multisample_state_create_info = {VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
    multisample_state_create_info.sampleShadingEnable = VK_FALSE;
    multisample_state_create_info.rasterizationSamples = (VkSampleCountFlagBits)desc.rasterState.SampleCount;
    multisample_state_create_info.minSampleShading = 1.0f;
    multisample_state_create_info.pSampleMask = 0;
    multisample_state_create_info.alphaToCoverageEnable = VK_FALSE;
    multisample_state_create_info.alphaToOneEnable = VK_FALSE;

    // Depth stencil
    VkPipelineDepthStencilStateCreateInfo depth_stencil_state_create_info = {};
    depth_stencil_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depth_stencil_state_create_info.depthTestEnable = desc.depthStencilState.enableDepthTest;
    depth_stencil_state_create_info.depthWriteEnable = desc.depthStencilState.enableDepthWrite;
    depth_stencil_state_create_info.depthCompareOp = ConvertCompareOp(desc.depthStencilState.depthCompareOp);
    depth_stencil_state_create_info.stencilTestEnable = VK_FALSE; // TODO: Support stencil test and depth bounds test
    depth_stencil_state_create_info.depthBoundsTestEnable = VK_FALSE;

    // Blend state
    std::vector<VkPipelineColorBlendAttachmentState> color_blend_attachment_states(desc.blendState.attachments.size());
    for (size_t i = 0; i < color_blend_attachment_states.size(); ++i) {
        auto& color_blend_state = desc.blendState;
        color_blend_attachment_states[i].blendEnable = (color_blend_state.attachments[i].enable_blend? VK_TRUE : VK_FALSE);
        color_blend_attachment_states[i].srcColorBlendFactor = _ConvertBlendFactor(color_blend_state.attachments[i].srcColorBlendFactor);
        color_blend_attachment_states[i].dstColorBlendFactor = _ConvertBlendFactor(color_blend_state.attachments[i].dstColorBlendFactor);
        color_blend_attachment_states[i].colorBlendOp = _ConvertBlendOp(color_blend_state.attachments[i].colorBlendOp);
        color_blend_attachment_states[i].srcAlphaBlendFactor = _ConvertBlendFactor(color_blend_state.attachments[i].srcAlphaBlendFactor);
        color_blend_attachment_states[i].dstAlphaBlendFactor = _ConvertBlendFactor(color_blend_state.attachments[i].srcAlphaBlendFactor);
        color_blend_attachment_states[i].alphaBlendOp = _ConvertBlendOp(color_blend_state.attachments[i].alphaBlendOp);
        color_blend_attachment_states[i].colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                                    VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    }

    VkPipelineColorBlendStateCreateInfo color_blend_state_create_info = {};
    color_blend_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    color_blend_state_create_info.logicOpEnable = VK_FALSE; // TODO: Support logic operation
    color_blend_state_create_info.logicOp = VK_LOGIC_OP_MAX_ENUM;
    color_blend_state_create_info.attachmentCount = color_blend_attachment_states.size();
    color_blend_state_create_info.pAttachments = color_blend_attachment_states.data();
    color_blend_state_create_info.blendConstants[0] = 1.0f; //TODO: Make this configurable
    color_blend_state_create_info.blendConstants[1] = 1.0f;
    color_blend_state_create_info.blendConstants[2] = 1.0f;
    color_blend_state_create_info.blendConstants[3] = 1.0f;

    // Dynamic state
    const u32 dynamic_state_count = 2;
    VkDynamicState dynamic_states[dynamic_state_count] = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo dynamic_state_create_info = {VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};
    dynamic_state_create_info.dynamicStateCount = dynamic_state_count;
    dynamic_state_create_info.pDynamicStates = dynamic_states;

    // Rendering info : we are using dynamic rendering instead of renderpass and framebuffer
    const auto& renderPassInfo = desc.renderPassInfo;
    std::vector<VkFormat> vk_color_attachment_format;
    vk_color_attachment_format.resize(renderPassInfo.numColorAttachments);
    for (size_t i = 0; i < desc.renderPassInfo.numColorAttachments; i++) {
        vk_color_attachment_format[i] = ConvertDataFormat(renderPassInfo.colorAttachmentFormats[i]);
    }
    
    VkPipelineRenderingCreateInfo renderingInfo = {};
    renderingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
    renderingInfo.colorAttachmentCount = vk_color_attachment_format.size();
    renderingInfo.pColorAttachmentFormats = vk_color_attachment_format.data();
    if (IsFormatSupportDepth(renderPassInfo.depthAttachmentFormat)) {
        renderingInfo.depthAttachmentFormat = ConvertDataFormat(renderPassInfo.depthAttachmentFormat);
    }

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
	pipeline_create_info.layout = m_Layout->handle;
    pipeline_create_info.pNext = &renderingInfo;
    pipeline_create_info.renderPass = nullptr;
    VK_CHECK(vkCreateGraphicsPipelines(m_Device->vkDevice, nullptr, 1, &pipeline_create_info, nullptr, &m_Handle))

    CORE_LOGD("Graphic Pipeline created")
}

PipeLine_Vulkan::~PipeLine_Vulkan()
{
    if (m_Handle != VK_NULL_HANDLE) {
        auto& frame = m_Device->GetCurrentFrame();
        frame.garbagePipelines.push_back(m_Handle);
    }

    CORE_LOGD("Vulkan pipeline destroyed")
}

}