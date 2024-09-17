#pragma once
#include <Quark/Core/Application.h>
#include <Quark/Scene/Scene.h>
#include <Quark/Renderer/SceneRenderer.h>

namespace quark {
    class TestBed : public Application {
public:
    TestBed(const ApplicationSpecification& specs);
    ~TestBed();

    virtual void OnUpdate(TimeStep deltaTime) override final;
    virtual void OnRender(TimeStep deltaTime) override final;
    virtual void OnImGuiUpdate() override final;

    void CreateDepthImage();
    void CreateRenderPassInfos();
    void LoadScene();

    Ref<graphic::Shader> vert_shader;
    Ref<graphic::Shader> frag_shader;
    Ref<graphic::PipeLine> graphic_pipeline;

    graphic::RenderPassInfo2 geometry_pass_info; // First pass
    graphic::RenderPassInfo2 ui_pass_info;   // Second pass

    Ref<graphic::Image> depth_image;
    graphic::DataFormat depth_format = graphic::DataFormat::D32_SFLOAT;

    Ref<Scene> scene;
    Scope<SceneRenderer> scene_renderer;

    // Debug
    float cmdListRecordTime = 0;
};

}