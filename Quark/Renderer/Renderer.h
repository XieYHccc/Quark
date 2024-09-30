#pragma once
#include "Quark/Core/Util/Singleton.h"
#include "Quark/Graphic/Device.h"
#include "Quark/Renderer/ShaderLibrary.h"
namespace quark {

// This class is responsible for managing some global gpu resources
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
    graphic::RenderPassInfo2 renderPassInfo2_simpleColorDepthPass;
    graphic::RenderPassInfo2 renderPassInfo2_uiPass;

    Renderer() = default;
    ~Renderer() = default;

    void Init();
    void Shutdown();

    ShaderLibrary& GetShaderLibrary() { return *m_ShaderLibrary; }

private:
    Scope<ShaderLibrary> m_ShaderLibrary;
};
}