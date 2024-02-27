#pragma once

#include "../render/window.h"
#include "../render/camera.h"
#include "../render/shader.h"
#include "../render/mesh_renderer.h"

class Viewer final : public Window {
public:
	Viewer(const char* title, int width, int height);
	~Viewer();
	
private:
	void render() override;
	void process_input(GLFWwindow* window) override;
	void motion(double /*xpos*/, double /*ypos*/) override;

private:
	// Camera attributes
	float lastX_;
	float lastY_;
	bool first_mouse_;

	// Frame attribute
	float deltaTime_;
	float lastFrame_;

};