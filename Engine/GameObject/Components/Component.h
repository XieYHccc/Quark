#pragma  once
#include "Util/CRC32.h"

class GameObject;
class Component {
public:
    Component(GameObject* owner) : owner_(owner) {};
    virtual ~Component() {};

    virtual std::uint32_t GetType() const = 0;
    virtual void Awake() {};
    virtual void Update() {};

    GameObject* GetOwner() { return owner_; }

private:
    GameObject* owner_;
    
};

#define COMPONENT_TYPE(componentType)               \
    static constexpr std::uint32_t GetStaticType()  \
    {                                               \
        return CRC32(componentType);                \
    }                                               \
    std::uint32_t GetType() const override          \
    {                                               \
        return GetStaticType();                     \
    }