#pragma once

#include "Window.h"
#include "Camera.h"
#include "Shader.h"
#include "../Model.h"

class Viewer final : public Window {
public:
	Viewer(const char* title, int width, int height);

private:
	void render() override;
	void process_input(GLFWwindow* window) override;
	void motion(double /*xpos*/, double /*ypos*/) override;

private:
	// Camera attributes
	Camera cam_;
	float lastX_;
	float lastY_;
	bool first_mouse_;

	// Frame attribute
	float deltaTime_;
	float lastFrame_;


public:
	Shader shader_;

	// Scene
	std::vector<Model*> render_queue;
	

};