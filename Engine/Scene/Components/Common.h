#pragma once
#include "Scene/Ecs.h"

namespace scene {

struct NameCmpt : public Component
{
    using Component::Component;
    QK_COMPONENT_TYPE_DECL(NameCmpt)
    std::string name;
};

}