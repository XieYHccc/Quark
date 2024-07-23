#pragma once
#include "Scene/Ecs.h"

namespace scene {

struct NameCmpt : public Component
{
    NameCmpt(Entity* entity, const std::string& name) : Component(entity), name(name) {}
    NameCmpt(Entity* entity) : Component(entity) {};
    QK_COMPONENT_TYPE_DECL(NameCmpt)
    std::string name;
};

}