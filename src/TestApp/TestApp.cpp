#include "TestApp.h"

Application* CreateApplication()
{
    auto application = new TestApp("test"," ", 1300, 800);
    application->context.Init();
    return application;
}

TestApp::~TestApp()
{
    context.Finalize();
}

void TestApp::Update()
{
}

void TestApp::Render()
{   
   
}