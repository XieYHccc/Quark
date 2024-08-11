#pragma once
#include "Quark/Core/Base.h"
#include "Quark/Ecs/Entity.h"

namespace quark {
class RelationshipCmpt : public Component{
public:
    using Component::Component;
    QK_COMPONENT_TYPE_DECL(RelationshipCmpt)

    Entity* GetParentEntity() const { return m_ParentEntity; }
    std::vector<Entity*>& GetChildEntities() { return m_ChildEntities; }

    void AddChildEntity(Entity* child) ;

    void RemoveChildEntity(Entity* child);

private:
    Entity* m_ParentEntity = nullptr;
    std::vector<Entity*> m_ChildEntities;
};
}