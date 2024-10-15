#pragma once
#include "Quark/Core/UUID.h"
#include "Quark/Ecs/Component.h"

#include <string>

namespace quark {

struct NameCmpt : public Component
{
    std::string name;
    NameCmpt(const std::string& name) : name(name) {}
    NameCmpt() = default;
    QK_COMPONENT_TYPE_DECL(NameCmpt)
};

struct IdCmpt : public Component
{
    UUID id;
    using Component::Component;
    QK_COMPONENT_TYPE_DECL(IdComponent)
    
};

} // namespace quark