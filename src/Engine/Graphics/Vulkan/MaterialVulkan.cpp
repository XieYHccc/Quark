#include "Graphics/Vulkan/MaterialVulkan.h"

#include "Graphics/Vulkan/CreateInfoUtils.h"
#include "Graphics/Vulkan/PipelineBuilder.h"
#include "Graphics/Vulkan/RendererVulkan.h"


void GLTFMetallic_Roughness::BuildPipelines()
{
	VkShaderModule meshFragShader = RendererVulkan::GetInstance()->LoadShader("/Users/xieyhccc/develop/XEngine/src/Assets/Shaders/Spirv/mesh.frag.spv");
	VkShaderModule meshVertexShader = RendererVulkan::GetInstance()->LoadShader("/Users/xieyhccc/develop/XEngine/src/Assets/Shaders/Spirv/mesh.vert.spv");

	// build material descriptor set layout
    DescriptorSetLayoutBuilder layoutBuilder;
    layoutBuilder.AddBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);
    layoutBuilder.AddBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);
	layoutBuilder.AddBinding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);
    materialLayout = layoutBuilder.Build();

	// create push constants range
	VkPushConstantRange matrixRange{};
	matrixRange.offset = 0;
	matrixRange.size = sizeof(GpuDrawPushConstants);
	matrixRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

	// Build Pipeline layout
	VkDescriptorSetLayout layouts[] = { RendererVulkan::GetInstance()->GetSceneDescriptorSetLayout(),
        materialLayout };

	VkPipelineLayoutCreateInfo mesh_layout_info = vkutil::pipeline_layout_create_info();
	mesh_layout_info.setLayoutCount = 2;
	mesh_layout_info.pSetLayouts = layouts;
	mesh_layout_info.pPushConstantRanges = &matrixRange;
	mesh_layout_info.pushConstantRangeCount = 1;
	VkPipelineLayout newLayout;
	if (vkCreatePipelineLayout(RendererVulkan::GetInstance()->GetVkDevice(), &mesh_layout_info, nullptr, &newLayout))
		XE_CORE_ERROR("GLTFMetallic_Roughness::BuildPipelines() : Faile to create pipeline layout")
	
	// Build Pipeline
	PipelineBuilder pipelineBuilder;
	pipelineBuilder.setShaders(meshVertexShader, meshFragShader);
	pipelineBuilder.setInputTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
	pipelineBuilder.setPolygonMode(VK_POLYGON_MODE_FILL);
	pipelineBuilder.setCullMode(VK_CULL_MODE_NONE, VK_FRONT_FACE_CLOCKWISE);
	pipelineBuilder.setMultiSamplingNone();
	pipelineBuilder.disableBlending();
	pipelineBuilder.enableDepthtest(true, VK_COMPARE_OP_GREATER_OR_EQUAL);
	pipelineBuilder.setColorAttachmentFormat(VK_FORMAT_R16G16B16A16_SFLOAT);
	pipelineBuilder.setDepthAttachmentFormat(VK_FORMAT_D32_SFLOAT);
	pipelineBuilder.pipelineLayout = newLayout;

	// build opaque pipeline
    opaquePipeline.pipeline = pipelineBuilder.BuildPipeline();

	// build transparent pipeline
	pipelineBuilder.enableBlendingAdditive();
	pipelineBuilder.enableDepthtest(false, VK_COMPARE_OP_GREATER_OR_EQUAL);
	transparentPipeline.pipeline = pipelineBuilder.BuildPipeline();

	// set pipeline layout
	opaquePipeline.layout = newLayout;
	transparentPipeline.layout = newLayout;
	
	// set pipeline layout
	vkDestroyShaderModule(RendererVulkan::GetInstance()->GetVkDevice(), meshVertexShader, nullptr);
	vkDestroyShaderModule(RendererVulkan::GetInstance()->GetVkDevice(), meshFragShader, nullptr);
}

std::unique_ptr<GpuMaterialInstance> GLTFMetallic_Roughness::CreateInstance(MATERIAL_PASS_TYPE pass, const MaterialResources& resources, DescriptorAllocator& descriptorAllocator)
{
	auto matData = std::make_unique<GpuMaterialInstance>();
	matData->passType = pass;
	if (pass == MATERIAL_PASS_TYPE::TRANSPARENT)
		matData->pipeline = &transparentPipeline;
	else 
		matData->pipeline = &opaquePipeline;

	matData->materialSet = descriptorAllocator.Allocate(materialLayout);


	writer.Clear();
	writer.WriteBuffer(0, resources.dataBuffer, sizeof(MaterialConstants), resources.dataBufferOffset, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
	writer.WriteImage(1, resources.colorImage.imageView, resources.colorSampler, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
	writer.WriteImage(2, resources.metalRoughImage.imageView, resources.metalRoughSampler, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

	writer.UpdateSet(matData->materialSet);

	return matData;
}

void GLTFMetallic_Roughness::ClearResources()
{
	VkDevice vkdevice = RendererVulkan::GetInstance()->GetVkDevice();
	vkDestroyPipeline(vkdevice, opaquePipeline.pipeline, nullptr);
	vkDestroyPipeline(vkdevice, transparentPipeline.pipeline, nullptr);
	vkDestroyPipelineLayout(vkdevice, transparentPipeline.layout, nullptr);
	vkDestroyDescriptorSetLayout(vkdevice, materialLayout, nullptr);
}

