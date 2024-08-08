#pragma once
#include "Quark/Core/Base.h"
#include "Quark/Core/Util/CompileTimeHash.h"

namespace quark {

using ComponentType = uint64_t;

class Entity;
class Component {
    friend class EntityRegistry;
public:
    Component() = default;
    virtual ComponentType GetType() = 0;
    virtual ~Component() {};
    Entity* GetEntity() { return m_Entity; }

private:
    Entity* m_Entity;
};

#define QK_COMPONENT_TYPE_DECL(x) \
static inline constexpr ComponentType GetStaticComponentType() { \
    return ComponentType(util::compile_time_fnv1(#x)); \
}\
ComponentType GetType() override {\
    return GetStaticComponentType();\
}

template <typename... Ts>
using ComponentGroup = std::tuple<Ts*...>;

template <typename... Ts>
using ComponentGroupVector = std::vector<ComponentGroup<Ts...>>;

template <typename... Ts>
constexpr uint64_t GetComponentGroupId()
{
    return util::compile_time_fnv1_merged(Ts::GetStaticComponentType()...);
}

}