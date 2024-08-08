#pragma once
#include <functional>
#include "Quark/Events/Event.h"

namespace quark {

template<typename T, typename = std::enable_if_t<std::is_base_of_v<Event, T>>>
using EventCallbackFn = std::function<void(const T& e)>;

class BaseEventHandler {
public:

    virtual ~BaseEventHandler() = default;

    virtual std::string GetName() const = 0;

    void Exec(const Event& e) { Call(e); }

private:
    virtual void Call(const Event& e) = 0;

};

template<typename T>
class EventHandler : public BaseEventHandler {
public:
    explicit EventHandler(const EventCallbackFn<T>& func)
        : callbackfunc_(func) {}

    std::string GetName() const override { return callbackfunc_.target_type().name(); }

private:
    void Call(const Event& e) override
    {
        if (e.GetEventType() == T::GetStaticEventType()) {
            callbackfunc_(static_cast<const T&>(e));
        }
    }
    
    EventCallbackFn<T> callbackfunc_;
};

}