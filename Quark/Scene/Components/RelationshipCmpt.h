#pragma once
#include "Quark/Core/Base.h"
#include "Quark/Ecs/Entity.h"

namespace quark {
class RelationshipCmpt : public Component{
public:
    using Component::Component;
    QK_COMPONENT_TYPE_DECL(RelationshipCmpt)

    Entity* GetParentEntity() const { return m_parentEntity; }
    std::vector<Entity*>& GetChildEntities() { return m_childEntities; }

    void AddChildEntity(Entity* child) ;

    void RemoveChildEntity(Entity* child);

private:
    Entity* m_parentEntity = nullptr;
    std::vector<Entity*> m_childEntities;
};
}