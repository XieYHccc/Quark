#pragma once
#include "Quark/Core/UUID.h"
#include "Quark/Ecs/Entity.h"

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
    UUID uuid;
    using Component::Component;
    QK_COMPONENT_TYPE_DECL(IdComponent)
    
};

class GameObject;
struct RelationshipCmpt : public Component
{
    GameObject* parent = nullptr;
    std::vector<GameObject*> children;
    using Component::Component;
    QK_COMPONENT_TYPE_DECL(RelationshipCmpt)
};


} // namespace quark