#pragma once
#include "Util/CompileTimeHash.h"

namespace scene {
using ComponentType = u64;

class Entity;
class Component {
public:
    Component(Entity* entity) : entity_(entity) {};
    virtual ~Component() {};

    virtual ComponentType GetType() = 0;
    virtual void Awake() {};
    virtual void Update() {};
    Entity* GetEntity() { return entity_; }

private:
    Entity* entity_;
};

#define QK_COMPONENT_TYPE_DECL(x) \
static inline constexpr ComponentType GetStaticComponentType() { \
    return ComponentType(util::compile_time_fnv1(#x)); \
}\
ComponentType GetType() override {\
    return GetStaticComponentType();\
}

}