#include "pch.h"
#include "Events/EventManager.h"


void EventManager::ImmediateTrigger(const Event &evnet)
{
    CORE_LOGT("{}", evnet.ToString());
    TriggerEvent(evnet);
}

void EventManager::TriggerEvent(const Event &event) 
{
    for (auto& subscriber : subscribers_[event.GetEventType()])
    {
        if (!event.isHandled)
            subscriber->Exec(event);
        else
            break;
    }
}

void EventManager::DispatchEvents() 
{
    for (auto eventItr = event_queue_.begin(); eventItr != event_queue_.end();) {
        CORE_LOGT("{}", eventItr->get()->ToString());
        TriggerEvent(*(eventItr->get()));
        eventItr = event_queue_.erase(eventItr);
    }
}