#pragma once

#include <sstream>
#include <string>
#include <typeindex>

class Event {
public:
    virtual ~Event() = default;
    virtual std::type_index GetEventType() const = 0;
    virtual std::string ToString() const  = 0;

    bool isHandled { false };
};

#define EVENT_TYPE(event_type)                      \
    static std::type_index GetStaticEventType()     \
    {                                               \
        return std::type_index(typeid(event_type)); \
    }                                               \
    std::type_index GetEventType() const override   \
    {                                               \
        return GetStaticEventType();                \
    }

inline std::ostream& operator<<(std::ostream& os, const Event& e)
{
    return os << e.ToString();
}
