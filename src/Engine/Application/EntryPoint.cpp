#include "Application/EntryPoint.h"
#include "Foundation/Log/Logger.h"
int main(int argc, char** argv)
{
    Logger::Init();
    XE_CORE_INFO("Init Engine")
    auto app = CreateApplication();
    app->Run();
    delete app;

    return 0;
}