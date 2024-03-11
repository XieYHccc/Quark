#pragma once

#include "Events/Event.h"

#include <functional>
template<typename EventType>
using EventCallbackFn = std::function<void(const EventType& e)>;

class BaseEventHandler {
public:

    virtual ~BaseEventHandler() = default;

    virtual std::string GetName() const = 0;

    void Exec(const Event& e) { Call(e); }

private:
    virtual void Call(const Event& e) = 0;

};

template<typename EventType>
class EventHandler : public BaseEventHandler {
public:
    explicit EventHandler(const EventCallbackFn<EventType>& func)
        : callbackfunc_(func) {}

    std::string GetName() const override { return callbackfunc_.target_type().name(); }

private:
    // type check has been done in event manager.
    void Call(const Event& e) override { callbackfunc_(static_cast<EventType&>(e)); }
    
    EventCallbackFn<EventType> callbackfunc_;
};