#include "EventManager.h"

void EventManager::DispatchEvents() 
{
    for (auto eventItr = event_queue_.begin(); eventItr != event_queue_.end();) {
        if (!eventItr->get()->isHandled) {
            TriggerEvent(*eventItr->get());
            eventItr = m_eventsQueue.erase(eventIt);
        } else {
            ++eventIt;
        }
    }
}