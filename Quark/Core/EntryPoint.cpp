#include "Core/EntryPoint.h"

int main(int argc, char** argv)
{
    auto app = CreateApplication();
    app->Run();
    delete app;

    return 0;
}