#pragma once
#include <glm/glm.hpp>

namespace quark {

class UIWindowBase {
public:
    UIWindowBase() = default;
    virtual ~UIWindowBase() = default;
    virtual void Render() = 0;
    
};

bool DrawVec3Control(const char* label, glm::vec3& vector, float reset = 0.f, float columnWidth = 100.f);

}