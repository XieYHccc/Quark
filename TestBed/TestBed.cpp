#include "TestBed/TestBed.h"


TestBed::TestBed(const std::string& title, const std::string& root, int width, int height)
    : Application(title, root, width, height)
{
    
}

Application* CreateApplication()
{
    return new TestBed("TestBed"," ", 1200, 1000);
}

TestBed::~TestBed()
{

}

void TestBed::Update()
{
}

void TestBed::Render()
{
}