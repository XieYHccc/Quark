#pragma once
#include <glm/glm.hpp>

namespace quark {

class Panel {
public:
    Panel() = default;
    virtual ~Panel() = default;
    virtual void OnImGuiUpdate() = 0;
    
};

}