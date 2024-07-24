#pragma once
#include <glm/glm.hpp>

namespace editor::ui {

class UIWindowBase {
public:
    UIWindowBase() = default;
    virtual ~UIWindowBase() = default;
    virtual void Init()  = 0;
    virtual void Render() = 0;
    
};

bool DrawVec3Control(const char* label, glm::vec3& vector, float reset = 0.f, float columnWidth = 100.f);
bool DrawVec3Input(const char* label, glm::vec3& vector);
}