#pragma once
#include "Core/InputManager.h"
#include <GLFW/glfw3.h>

class InputManagerGLFW : public InputManager {
public:
    void Init() override;
    void Update() override;
    void Finalize() override;

private:
    friend class WindowGLFW;
    friend class MakeSingletonPtr<InputManager>;

    InputManagerGLFW() = default;

    void RecordKey(int key, int action);
    void RecordMousePosition(float xpos, float ypos);

    GLFWwindow* window_ = nullptr;
};