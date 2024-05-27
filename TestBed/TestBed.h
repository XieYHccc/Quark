#pragma once
#include <Engine/Core/Application.h>

class Scene;

class TestBed : public Application 
{  
public:
    TestBed(const std::string& title, const std::string& root, int width, int height);
    ~TestBed();

    void Update() override;
    void Render() override;

};
