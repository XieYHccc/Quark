#pragma once
#include <Engine/Core/Application.h>
#include <Engine/Scene/NewScene.h>

class TestBed : public Application 
{  
public:
    TestBed(const std::string& title, const std::string& root, int width, int height);
    ~TestBed();

    virtual void Update(f32 deltaTime) override final;
    virtual void Render(f32 deltaTime) override final;

    Ref<graphic::Shader> vert_shader;
    Ref<graphic::Shader> frag_shader;
    Ref<graphic::PipeLine> graphic_pipeline;

    Scope<scene::Scene> scene;
};
