#include "Quark/qkpch.h"
#include "Quark/Events/EventManager.h"

namespace quark {

void EventManager::TriggerEvent(const Event &event) 
{
    CORE_LOGT("{}", event.ToString());

    for (auto& subscriber : subscribers_[event.GetEventType()])
    {
        if (!event.isHandled)
            subscriber->Exec(event);
        else
            return; // If the event is handled, stop send to other subscribers
    }
}

void EventManager::DispatchEvents() 
{
    for (auto eventItr = event_queue_.begin(); eventItr != event_queue_.end();) {
        // CORE_LOGT("{}", eventItr->get()->ToString());
        TriggerEvent(*(eventItr->get()));
        eventItr = event_queue_.erase(eventItr);
    }
}

}