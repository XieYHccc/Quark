#pragma once
#include <Engine/Core/Application.h>

class Scene;

class TestBed : public Application 
{  
public:
    TestBed(const std::string& title, const std::string& root, int width, int height);
    ~TestBed();

    virtual void Update(f32 deltaTime) override final;
    virtual void Render(f32 deltaTime) override final;

};
