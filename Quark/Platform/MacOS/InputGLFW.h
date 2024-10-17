#pragma once
#include "Quark/Core/Input.h"

#include <GLFW/glfw3.h>

namespace quark {

class InputGLFW : public Input {
public:
    void Init() override;
    void OnUpdate() override;
    void Finalize() override;

private:
    friend class WindowGLFW;
    friend class MakeSingletonPtr<Input>;

    InputGLFW() = default;

    void RecordKey(int key, int action);
    void RecordMousePosition(float xpos, float ypos);
};

}