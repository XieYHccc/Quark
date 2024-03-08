#pragma once

#include <string>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

class Application {
public:
    Application() {}
    ~Application();

    void Initialize(const std::string& title, int width, int height);
    void Finalize();
    void Run();

private:
    void Update();
    void Render();

private:

    GLFWwindow* window_;
    std::string title_;
    int width_, height_;

    // per-frame logic
    float delta_time_;
    float last_frame_;

};
