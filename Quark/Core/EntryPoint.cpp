#include "Quark/Core/EntryPoint.h"

int main(int argc, char** argv)
{
    auto app = quark::CreateApplication();
    app->Run();
    delete app;

    return 0;
}
