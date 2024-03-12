#include "EventManager.h"

void EventManager::TriggerEvent(const Event &event) 
{
    for (auto& subscriber : subscribers_[event.GetEventType()])
        subscriber->Exec(event);
}

void EventManager::DispatchEvents() 
{
    for (auto eventItr = event_queue_.begin(); eventItr != event_queue_.end();) {
        if (!eventItr->get()->isHandled) {
            TriggerEvent(*(eventItr->get()));
            eventItr = event_queue_.erase(eventItr);
        } 
        else {
            ++eventItr;
        }
    }
}