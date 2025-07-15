#pragma once
#include "Quark/Render/ShaderLibrary.h"
#include "Quark/Render/Mesh.h"

#include "Quark/RHI/Device.h"

namespace quark
{
struct ImageAsset;
class RenderResourceManager
{
public:
	// formats
	rhi::DataFormat format_depthAttachment_main = rhi::DataFormat::D32_SFLOAT;
	rhi::DataFormat format_colorAttachment_main = rhi::DataFormat::R16G16B16A16_SFLOAT;

	// stencil states
	rhi::PipelineDepthStencilState depthStencilState_depthWrite;
	rhi::PipelineDepthStencilState depthStencilState_disabled;
	rhi::PipelineDepthStencilState depthStencilState_depthTestOnly;

	// color blending states
	rhi::PipelineColorBlendState blendState_opaque;
	rhi::PipelineColorBlendState blendState_transparent;

	// rasterization states
	rhi::RasterizationState rasterizationState_fill;
	rhi::RasterizationState rasterizationState_wireframe;

	// blend states
	rhi::PipelineColorBlendState colorBlendState_opaque;
	rhi::PipelineColorBlendState colorBlendState_transparent;

	// vertex input layout
	uint32_t mesh_attrib_mask_skybox;
	rhi::VertexInputLayout vertexInputLayout_skybox;

	// renderPassInfo
	rhi::RenderPassInfo2 renderPassInfo_swapchainPass;
	rhi::RenderPassInfo2 renderPassInfo_simpleMainPass;
	rhi::RenderPassInfo2 renderPassInfo_editorMainPass;
	rhi::RenderPassInfo2 renderPassInfo_entityIdPass;

	// default images
	Ref<rhi::Image> image_white;
	Ref<rhi::Image> image_black;
	Ref<rhi::Image> image_checkboard;

	// default samplers
	Ref<rhi::Sampler> sampler_linear;
	Ref<rhi::Sampler> sampler_nearst;
	Ref<rhi::Sampler> sampler_cube;

	// default materials
	Ref<PBRMaterial> default_material;

	// pipelines
	//Ref<rhi::PipeLine> pipeline_skybox;
	//Ref<rhi::PipeLine> pipeline_infiniteGrid;
	Ref<rhi::PipeLine> pipeline_entityID;
		
	RenderResourceManager(Ref<rhi::Device> device);

	ShaderLibrary& GetShaderLibrary() { return *m_shader_library; }

	std::vector<Ref<IRenderable>>   RequestStaticMeshRenderables(Ref<MeshAsset> mesh_asset);
	Ref<MeshBuffers>				RequestMeshBuffers(Ref<MeshAsset> mesh_asset);
	Ref<PBRMaterial>				RequestMateral(Ref<MaterialAsset> mat_asset);
	Ref<rhi::PipeLine>				RequestGraphicsPSO(ShaderProgram& program, const rhi::RenderPassInfo2& rp, const uint32_t mesh_attrib_mask, bool enableDepth, DrawPipeline draw_pipeline);
	Ref<rhi::Image>					RequestImage(Ref<ImageAsset> image_asset);
	rhi::VertexInputLayout&			RequestMeshVertexLayout(uint32_t meshAttributesMask);

private:
	Ref<rhi::Device> m_device;
	Scope<ShaderLibrary> m_shader_library;

	// cached render resources
	std::unordered_map<uint64_t, rhi::VertexInputLayout> m_mesh_vertex_layouts;
	std::unordered_map<uint64_t, Ref<rhi::Image>> m_images;
	std::unordered_map<uint64_t, Ref<rhi::PipeLine>> m_cached_psos;
	std::unordered_map<uint64_t, Ref<IRenderable>> m_renderables;
	std::unordered_map<uint64_t, Ref<PBRMaterial>> m_materials;
	std::unordered_map<uint64_t, std::vector<Ref<IRenderable>>> m_static_meshes;
	std::unordered_map<uint64_t, Ref<MeshBuffers>> m_mesh_buffers;
};

}
