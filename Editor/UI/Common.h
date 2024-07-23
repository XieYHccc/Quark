#pragma once
#include <glm/glm.hpp>

namespace editor::ui {

class Window {
public:
    Window() = default;
    virtual ~Window() = default;
    virtual void Render() = 0;
    
};

bool DrawVec3Control(const char* label, glm::vec3& vector, float reset = 0.f, float columnWidth = 100.f);

}