#pragma once
#include "Quark/Core/Base.h"
#include "Quark/Core/Util/CompileTimeHash.h"

namespace quark {

using ComponentType = uint64_t;

class Entity;

// You can not create a component directly, you have to create a component by calling Entity::AddComponent<T>()
// which will assign the entity to the component
class Component {
public:
    Component() = default;
    virtual ~Component() = default;
    virtual ComponentType GetType() = 0;
    Entity* GetEntity() { return m_Entity; }

private:
    Entity* m_Entity;
    friend class EntityRegistry;
};

#define QK_COMPONENT_TYPE_DECL(x) \
static inline constexpr ComponentType GetStaticComponentType() { \
    return ComponentType(util::compile_time_fnv1(#x)); \
}\
ComponentType GetType() override {\
    return GetStaticComponentType();\
}

template <typename T, typename Tup>
inline T* GetComponent(Tup& t)
{
    return std::get<T*>(t);
}

template <typename T>
inline T* Get(const std::tuple<T*>& t)
{
    return std::get<0>(t);
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