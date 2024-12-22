#pragma once
#include "Quark/Render/RenderScene.h"
#include "Quark/Render/ShaderLibrary.h"
#include "Quark/RHI/Device.h"

namespace quark
{
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
		uint64_t default_material_id;

		// pipelines
		//Ref<rhi::PipeLine> pipeline_skybox;
		//Ref<rhi::PipeLine> pipeline_infiniteGrid;
		Ref<rhi::PipeLine> pipeline_entityID;
		
		// buffers
		Ref<rhi::Buffer> ubo_scene;

		RenderResourceManager(Ref<rhi::Device> device);

		ShaderLibrary& GetShaderLibrary() { return *m_shaderLibrary; }

		// these function will overwrite the existing resouce
		// check if the asset is already registerd before calling these functions
		void CreateMeshRenderResouce(AssetID mesh_asset_id);
		void CreateMaterialRenderResource(AssetID material_asset_id);
		void CreateImageRenderResource(AssetID image_asset_id);

		bool IsMeshAssetRegisterd(AssetID mesh_id) const;
		bool IsMaterialAssetRegisterd(AssetID material_id) const;
		bool IsImageAssetRegisterd(AssetID image_id) const;

		RenderMesh& GetRenderMesh(uint64_t mesh_id);
		RenderPBRMaterial& GetRenderMaterial(uint64_t material_id);
		Ref<rhi::Image> GetImage(uint64_t image_id);

    	Ref<rhi::PipeLine> GetOrCreateGraphicsPSO(ShaderProgram& program, const rhi::RenderPassInfo2& rp, const uint32_t mesh_attrib_mask, bool enableDepth, AlphaMode mode);
		Ref<rhi::VertexInputLayout> GetOrCreateVertexInputLayout(uint32_t meshAttributesMask);
		rhi::VertexInputLayout& GetOrCreateMeshVertexLayout(uint32_t meshAttributesMask);

		void UpdatePerFrameBuffer(const Ref<RenderScene>& scene);
		void UpdateMeshRenderResource(AssetID mesh_id);

	private:
		Ref<rhi::Device> m_device;
		Scope<ShaderLibrary> m_shaderLibrary;

		// caching
		std::unordered_map<uint64_t, RenderMesh> m_render_meshes;
		std::unordered_map<uint64_t, RenderPBRMaterial> m_render_materials;
		std::unordered_map<uint64_t, rhi::VertexInputLayout> m_mesh_vertex_layouts;
		std::unordered_map<uint64_t, Ref<rhi::Image>> m_images;
		std::unordered_map<uint64_t, Ref<rhi::PipeLine>> m_cached_pipelines;
    	std::unordered_map<uint64_t, Ref<rhi::VertexInputLayout>> m_cached_vertexInputLayouts;

		// 
	};

}
