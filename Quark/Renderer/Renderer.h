#pragma once
#include "Quark/Core/Util/Singleton.h"
#include "Quark/Graphic/Device.h"
#include "Quark/Renderer/ShaderLibrary.h"
#include "Quark/Renderer/SceneRenderer.h"

namespace quark {

class Renderer : public util::MakeSingleton<Renderer> {
public:
    // defalut resources and settings
    graphic::DataFormat format_depthAttachment_main = graphic::DataFormat::D32_SFLOAT;
    graphic::DataFormat format_colorAttachment_main = graphic::DataFormat::R16G16B16A16_SFLOAT;

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

    // defalut depth stencil states
    graphic::PipelineDepthStencilState depthStencilState_depthWrite;
    graphic::PipelineDepthStencilState depthStencilState_disabled;
    graphic::PipelineDepthStencilState depthStencilState_depthTestOnly;

    // defalut rasterization states
    graphic::RasterizationState rasterizationState_fill;
    graphic::RasterizationState rasterizationState_wireframe;

    // vertex input layout
    graphic::VertexInputLayout vertexInputLayout_skybox;

    // graphic::RenderPassInfo
    graphic::RenderPassInfo2 renderPassInfo2_simpleColorPass;
    graphic::RenderPassInfo2 renderPassInfo2_simpleMainPass;
    graphic::RenderPassInfo2 renderPassInfo2_editorMainPass;
    graphic::RenderPassInfo2 renderPassInfo2_uiPass;

    Renderer(graphic::Device* device);
    ~Renderer();

    ShaderLibrary& GetShaderLibrary() { return *m_shaderLibrary; }

    // Scene Rendering
    void SetScene(Ref<Scene> scene);
    void SetSceneEnvironmentMap(Ref<Texture> cubeMap);
    void UpdateDrawContextEditor(const CameraUniformBufferBlock& cameraData);
    void UpdateDrawContext();

    void DrawSkybox(graphic::CommandList* cmd);
    void DrawScene(graphic::CommandList* cmd);

    Ref<graphic::PipeLine> GetOrCreatePipeLine(
        const ShaderProgramVariant& programVariant,
        const graphic::PipelineDepthStencilState& ds,
        const graphic::PipelineColorBlendState& cb,
        const graphic::RasterizationState& rs,
        const graphic::RenderPassInfo2& compatablerp,
        const graphic::VertexInputLayout& input);

private:
    graphic::Device* m_device;

    Scope<ShaderLibrary> m_shaderLibrary;
    Scope<SceneRenderer> m_sceneRenderer;

    std::unordered_map<uint64_t, Ref<graphic::PipeLine>> m_pipelines;

};
}