#pragma once

#include <sstream>
#include <string>
#include "Quark/Core/Util/CRC32.h"

namespace quark {

class Event {
public:
    using EventType = std::uint32_t;
    Event() {}
    virtual ~Event() = default;
    virtual std::uint32_t GetEventType() const = 0;
    virtual std::string ToString() const {return std::to_string(GetEventType());};

    bool isHandled { false };
};

#define EVENT_TYPE(event_type)                          \
    static constexpr std::uint32_t GetStaticEventType() \
    {                                                   \
        return CRC32(event_type);                       \
    }                                                   \
    std::uint32_t GetEventType() const override         \
    {                                                   \
        return GetStaticEventType();                    \
    }

inline std::ostream& operator<<(std::ostream& os, const Event& e)
{
    return os << e.ToString();
}

}