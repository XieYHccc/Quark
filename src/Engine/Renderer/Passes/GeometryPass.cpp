#include "Renderer/Passes/GeometryPass.h"
#include "Graphics/Vulkan/Initializers.h"
#include "Renderer/Renderer.h"
#include "Graphics/Vulkan/PipeLine.h"
#include "Scene/Scene.h"


GeometryPass::~GeometryPass()
{
    if (opaquePipeline_) {
        vkDestroyPipeline(Renderer::Instance().GetVkDevice(), opaquePipeline_, nullptr);
        vkDestroyPipeline(Renderer::Instance().GetVkDevice(), transparentPipeline_, nullptr);
    }
}

GeometryPass::GeometryPass(const char* vert_path, const char* frag_path,
    const std::vector<VkRenderingAttachmentInfo>& colorAttachments,
    const VkRenderingAttachmentInfo& depthAttachment)
{

    // Set attachments
    this->colorAttachmentsInfo_ = colorAttachments;
    this->depthAttachmentInfo_ = depthAttachment;

    // Create shader moudules
	VkShaderModule meshFragShader;
	VkShaderModule meshVertexShader;
	vk::LoadShaderModule(frag_path, Renderer::Instance().GetVkDevice(), &meshFragShader);
	vk::LoadShaderModule(vert_path, Renderer::Instance().GetVkDevice(), &meshVertexShader);

	// Build Pipeline
	vk::PipelineBuilder pipelineBuilder;
	pipelineBuilder.setShaders(meshVertexShader, meshFragShader);
	pipelineBuilder.setInputTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
	pipelineBuilder.setPolygonMode(VK_POLYGON_MODE_FILL);
	pipelineBuilder.setCullMode(VK_CULL_MODE_NONE, VK_FRONT_FACE_CLOCKWISE);
	pipelineBuilder.setMultiSamplingNone();
	pipelineBuilder.disableBlending();
	pipelineBuilder.enableDepthtest(true, VK_COMPARE_OP_GREATER_OR_EQUAL);
	pipelineBuilder.setColorAttachmentFormat(Renderer::Instance().GetColorFormat());
	pipelineBuilder.setDepthAttachmentFormat(Renderer::Instance().GetDepthFormat());
	pipelineBuilder.pipelineLayout = Renderer::Instance().GetGraphicsPipeLineLayout();

	// build opaque pipeline
    opaquePipeline_ = pipelineBuilder.BuildPipeline(Renderer::Instance().GetContext());

	// build transparent pipeline
	pipelineBuilder.enableBlendingAdditive();
	pipelineBuilder.enableDepthtest(false, VK_COMPARE_OP_GREATER_OR_EQUAL);
	transparentPipeline_ = pipelineBuilder.BuildPipeline(Renderer::Instance().GetContext());

	
	vkDestroyShaderModule(Renderer::Instance().GetVkDevice(), meshVertexShader, nullptr);
	vkDestroyShaderModule(Renderer::Instance().GetVkDevice(), meshFragShader, nullptr);
}

void GeometryPass::Prepare(Scene* scene)
{
    this->scene_ = scene;
    meshes_ = scene->GetComponents<MeshCmpt>();

    // prepare light information
    drawContext_.sceneData.ambientColor = glm::vec4(.1f);
	drawContext_.sceneData.sunlightColor = glm::vec4(1.f);
	drawContext_.sceneData.sunlightDirection = glm::vec4(0,1,0.5,1.f);
}

void GeometryPass::UpdateDrawContext()
{
	drawContext_.OpaqueObjects.clear();
	drawContext_.TransparentObjects.clear();

	// update render objects
	for (const auto& mesh : meshes_) {
        for (const auto& renderObject : mesh->GetRenderObjects()) {
            if (renderObject.material->GetAlphaMode() == AlphaMode::OPAQUE) {
                drawContext_.OpaqueObjects.push_back(renderObject);
            }
            else {
                drawContext_.TransparentObjects.push_back(renderObject);
            }
        }
    }

	auto mainCam = scene_->GetMainCamera();
	SceneBufferData& sceneData = drawContext_.sceneData;
	sceneData.view = mainCam->GetViewMatrix();
	sceneData.proj = mainCam->GetProjectionMatrix();
	sceneData.proj[1][1] *= -1;
	sceneData.viewproj = sceneData.proj * sceneData.view;

}

void GeometryPass::Draw(PerFrameData* frame)
{
    UpdateDrawContext();

    auto& renderer = Renderer::Instance();
    auto& context = renderer.GetContext();
    auto cmd = frame->mainCommandBuffer;
    VkRenderingInfo renderInfo = vk::init::rendering_info(resolution_, colorAttachmentsInfo_.data(), &depthAttachmentInfo_);
	renderer.GetContext().extendFunction.pVkCmdBeginRenderingKHR(cmd, &renderInfo);

    // allocate a new uniform buffer for the scene data
	vk::BufferBuilder bufferBuilder;
	bufferBuilder.SetSize(sizeof(SceneBufferData))
		.SetUsage(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT)
		.SetVmaUsage(VMA_MEMORY_USAGE_CPU_TO_GPU)
		.SetVmaFlags(VMA_ALLOCATION_CREATE_MAPPED_BIT);
	vk::Buffer sceneDataBuffer = bufferBuilder.Build(context);

	frame->deletionQueue.push_deletor([&, this, sceneDataBuffer]() {
		vk::Buffer::DestroyBuffer(context, sceneDataBuffer);
	});

	SceneBufferData* sceneData = (SceneBufferData*)sceneDataBuffer.info.pMappedData;
	*sceneData = drawContext_.sceneData;

    //create a descriptor set that binds that buffer and update it
	VkDescriptorSet globalDescriptor = frame->frameDescriptorPool.Allocate(renderer.GetSceneDescriptorSetLayout());
	vk::DescriptorWriter writer;
	writer.WriteBuffer(0, sceneDataBuffer.vkBuffer, sizeof(SceneBufferData), 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
	writer.UpdateSet(context, globalDescriptor);

    // GpuPipeLineVulkan* lastPipeline = nullptr;
	std::shared_ptr<asset::Material> lastMaterial = nullptr;
	VkBuffer lastIndexBuffer = VK_NULL_HANDLE;

	auto bindPipeLine= [&] (VkPipeline pipeline) {
		vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
		vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, renderer.GetGraphicsPipeLineLayout(), 0, 1,
			&globalDescriptor, 0, nullptr);

		VkViewport viewport = {};
		viewport.x = 0;
		viewport.y = 0;
		viewport.width = (float)resolution_.width;
		viewport.height = (float)resolution_.height;
		viewport.minDepth = 0.f;
		viewport.maxDepth = 1.f;

		vkCmdSetViewport(cmd, 0, 1, &viewport);

		VkRect2D scissor = {};
		scissor.offset.x = 0;
		scissor.offset.y = 0;
		scissor.extent.width = resolution_.width;
		scissor.extent.height = resolution_.height;

		vkCmdSetScissor(cmd, 0, 1, &scissor);
	};

    auto draw = [&](const RenderObject& r) {
		if (r.material != lastMaterial) {
			lastMaterial = r.material;
			vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, renderer.GetGraphicsPipeLineLayout(), 1, 1,
				&r.material->GetMaterialSet(), 0, nullptr);
		}
		//rebind index buffer if needed
		if (r.indexBuffer != lastIndexBuffer) {
			lastIndexBuffer = r.indexBuffer;
			vkCmdBindIndexBuffer(cmd, r.indexBuffer, 0, VK_INDEX_TYPE_UINT32);
		}
		// calculate final mesh matrix
		GpuDrawPushConstants push_constants;
		push_constants.worldMatrix = r.transform;
		push_constants.vertexBuffer = r.vertexBufferAddress;
		vkCmdPushConstants(cmd, renderer.GetGraphicsPipeLineLayout(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(GpuDrawPushConstants), &push_constants);

		vkCmdDrawIndexed(cmd, r.indexCount, 1, r.firstIndex, 0, 0);

	};

    // draw objects
	bindPipeLine(opaquePipeline_);
    for (const auto& object : drawContext_.OpaqueObjects) {
        draw(object);
    }

	bindPipeLine(transparentPipeline_);
	for (const auto& object : drawContext_.TransparentObjects) {
		draw(object);
	}

    // end rendering
    context.extendFunction.pVkCmdEndRenderingKHR(cmd);
}
