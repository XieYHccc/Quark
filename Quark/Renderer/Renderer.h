#pragma once
#include "Quark/Core/Util/Singleton.h"
#include "Quark/Graphic/Device.h"
#include "Quark/Renderer/ShaderLibrary.h"
#include "Quark/Renderer/SceneRenderer.h"

namespace quark {

// 1. high level rendering api
// 2. this class is a collection of graphics technique implentations and functions 
// to draw a scene, shadows, post processes and other things.
class Renderer : public util::MakeSingleton<Renderer> {
public:
    // formats
    graphic::DataFormat format_depthAttachment_main = graphic::DataFormat::D32_SFLOAT;
    graphic::DataFormat format_colorAttachment_main = graphic::DataFormat::R16G16B16A16_SFLOAT;

    // stencil states
    graphic::PipelineDepthStencilState depthStencilState_depthWrite;
    graphic::PipelineDepthStencilState depthStencilState_disabled;
    graphic::PipelineDepthStencilState depthStencilState_depthTestOnly;

    // color blending states
    graphic::PipelineColorBlendState blendState_opaque;
    graphic::PipelineColorBlendState blendState_transparent;

    // rasterization states
    graphic::RasterizationState rasterizationState_fill;
    graphic::RasterizationState rasterizationState_wireframe;

    // blend states
    graphic::PipelineColorBlendState colorBlendState_opaque;
    graphic::PipelineColorBlendState colorBlendState_transparent;

    // vertex input layout
    graphic::VertexInputLayout vertexInputLayout_skybox;

    // renderPassInfo
    graphic::RenderPassInfo2 renderPassInfo2_simpleColorPass;
    graphic::RenderPassInfo2 renderPassInfo2_simpleMainPass;
    graphic::RenderPassInfo2 renderPassInfo2_editorMainPass;
    graphic::RenderPassInfo2 renderPassInfo2_uiPass;

    // pipeline descs
    graphic::GraphicPipeLineDesc pipelineDesc_skybox;

    // default images
    Ref<graphic::Image> image_white;
    Ref<graphic::Image> image_black;
    Ref<graphic::Image> image_checkboard;

    // default samplers
    Ref<graphic::Sampler> sampler_linear;
    Ref<graphic::Sampler> sampler_nearst;
    Ref<graphic::Sampler> sampler_cube;

    // pipelines
    Ref<graphic::PipeLine> pipeline_skybox;

    struct PerFrameData 
    {
        std::vector<RenderObject> objects_opaque;
        std::vector<RenderObject> objects_transparent;
        Ref<graphic::Buffer> sceneUB;
    };

    struct Visibility
    {
        CameraUniformBufferBlock cameraData;

        std::vector<uint32_t> visible_opaque;
        std::vector<uint32_t> visible_transparent; 
        math::Frustum frustum;
    };

public:
    Renderer(graphic::Device* device);
    ~Renderer();

    ShaderLibrary& GetShaderLibrary() { return *m_shaderLibrary; }

    void SetScene(const Ref<Scene>& scene);

    void UpdateDrawContextEditor(const CameraUniformBufferBlock& cameraData);
    void UpdateDrawContext();

    void UpdatePerFrameData(const Ref<Scene>& scene, PerFrameData& perframeData);
    void UpdateVisibility(const CameraUniformBufferBlock& cameraData, const PerFrameData& perframeData, Visibility& vis);

    void DrawSkybox(const Ref<Texture>& envMap, graphic::CommandList* cmd);
    void DrawScene(graphic::CommandList* cmd);

    Ref<graphic::PipeLine> GetOrCreatePipeLine(
        const ShaderProgramVariant& programVariant,
        const graphic::PipelineDepthStencilState& ds,
        const graphic::PipelineColorBlendState& bs,
        const graphic::RasterizationState& rs,
        const graphic::RenderPassInfo2& rp,
        const graphic::VertexInputLayout& input);

private:
    graphic::Device* m_device;

    Scope<ShaderLibrary> m_shaderLibrary;
    Scope<SceneRenderer> m_sceneRenderer;

    std::unordered_map<uint64_t, Ref<graphic::PipeLine>> m_pipelines;

};
}