#pragma once

#include <string>
#include "Quark/Core/Util/CompileTimeHash.h"

namespace quark {

class Event {
public:
    using EventType = uint64_t;

    Event() = default;
    virtual ~Event() = default;

    virtual uint64_t GetEventType() const = 0;
    virtual std::string ToString() const {return std::to_string(GetEventType());};
    virtual void Log() const { QK_CORE_LOGT_TAG("EventManager", ToString()); }

    bool isHandled { false };
};

inline std::ostream& operator<<(std::ostream& os, const Event& e)
{
    return os << e.ToString();
}

}

#define EVENT_TYPE(event_type)                                   \
    static constexpr uint64_t GetStaticEventType()          \
    {                                                            \
        return ::quark::util::compile_time_fnv1(event_type);     \
    }                                                            \
    uint64_t GetEventType() const override                  \
    {                                                            \
        return GetStaticEventType();                             \
    }