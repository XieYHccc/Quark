#pragma once

#include "Quark/Core/Application.h"

extern quark::Application* quark::CreateApplication(int argc, char** argv);

int main(int argc, char** argv)
{
	quark::Application* app = quark::CreateApplication(argc, argv);
	app->Run();
	delete app;
}