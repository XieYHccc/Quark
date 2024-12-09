#pragma once
#include "Quark/Render/RenderTypes.h"
#include "Quark/Render/ShaderLibrary.h"
#include "Quark/RHI/Device.h"
#include "Quark/Core/Util/Singleton.h"
#include "Quark/Core/Math/Frustum.h"


namespace quark {
class Scene;

// 1. high level rendering api
// 2. a collection of graphics technique implentations and functions 
// to draw a scene, shadows, post processes and other things.
// 3. owner of the shader library
class Renderer : public util::MakeSingleton<Renderer> {
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
    rhi::VertexInputLayout vertexInputLayout_skybox;

    // renderPassInfo
    rhi::RenderPassInfo2 renderPassInfo_swapchainPass;
    rhi::RenderPassInfo2 renderPassInfo_simpleMainPass;
    rhi::RenderPassInfo2 renderPassInfo_editorMainPass;
    rhi::RenderPassInfo2 renderPassInfo_entityIdPass;

    // pipeline descs
    //rhi::GraphicPipeLineDesc pipelineDesc_skybox;
    //rhi::GraphicPipeLineDesc pipelineDesc_infiniteGrid;

    // default images
    Ref<rhi::Image> image_white;
    Ref<rhi::Image> image_black;
    Ref<rhi::Image> image_checkboard;

    // default samplers
    Ref<rhi::Sampler> sampler_linear;
    Ref<rhi::Sampler> sampler_nearst;
    Ref<rhi::Sampler> sampler_cube;

    // pipelines
    //Ref<rhi::PipeLine> pipeline_skybox;
    //Ref<rhi::PipeLine> pipeline_infiniteGrid;
    Ref<rhi::PipeLine> pipeline_entityID;

    struct DrawContext 
    {
        std::vector<RenderObject> objects_opaque;
        std::vector<RenderObject> objects_transparent;

        UniformBufferData_Scene sceneData;

        // gpu resources
        Ref<rhi::Buffer> sceneUB;
    };

    struct Visibility
    {
        UniformBufferData_Camera cameraData;
        math::Frustum frustum;
        std::vector<uint32_t> visible_opaque;
        std::vector<uint32_t> visible_transparent;
    };

public:
    Renderer(rhi::Device* device);
    ~Renderer();

    ShaderLibrary& GetShaderLibrary() { return *m_shaderLibrary; }

    // these two function is used to sync rendering data with the scene
    void UpdateDrawContext(const Ref<Scene>& scene, DrawContext& context);
    void UpdateVisibility(const DrawContext& context, Visibility& vis, const UniformBufferData_Camera& cameraData);
    
    void UpdateGpuResources(DrawContext& context, Visibility& vis);

    // update draw context and visibility before you call these draw functions
    void DrawSkybox(const DrawContext& context, const Ref<Texture>& envMap, rhi::CommandList* cmd);
    void DrawScene(const DrawContext& context, const Visibility& vis, rhi::CommandList* cmd);
    void DrawGrid(const DrawContext& context, rhi::CommandList* cmd);
    void DrawEntityID(const DrawContext& context, const Visibility& vis, rhi::CommandList* cmd);

    // caching
    Ref<rhi::PipeLine> GetGraphicsPipeline(const ShaderProgramVariant& programVariant, const rhi::PipelineDepthStencilState& ds, const rhi::PipelineColorBlendState& bs, const rhi::RasterizationState& rs, const rhi::RenderPassInfo2& rp, const rhi::VertexInputLayout& input);
    Ref<rhi::PipeLine> GetGraphicsPipeline(ShaderProgram& program, const ShaderVariantKey& key, const rhi::RenderPassInfo2& rp, const rhi::VertexInputLayout& vertexLayout, bool enableDepth, AlphaMode mode);
    Ref<rhi::VertexInputLayout> GetVertexInputLayout(uint32_t meshAttributesMask);

private:
    rhi::Device* m_device;

    Scope<ShaderLibrary> m_shaderLibrary;

    std::unordered_map<uint64_t, Ref<rhi::PipeLine>> m_cached_pipelines;
    std::unordered_map<uint32_t, Ref<rhi::VertexInputLayout>> m_cached_vertexInputLayouts;
};
}