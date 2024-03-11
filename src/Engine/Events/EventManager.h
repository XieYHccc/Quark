#pragma once

#include <iostream>

#include "EventHandler.h"

class EventManager {
public:

    void Initialize();
    void Finalize();

public:
    template<typename EventType, typename = std::enable_if_t<std::is_base_of_v<Event, EventType>>>
    void Subscribe(const EventCallbackFn<EventType>& func);

    template<typename EventType, typename = std::enable_if_t<std::is_base_of_v<Event, EventType>>>
    void Unsubscribe(const EventCallbackFn<EventType>& func);

    template<typename EventType, typename = std::enable_if_t<std::is_base_of_v<Event, EventType>>>
    void TriggerEvent(const EventType& evnet);

    template<typename EventType, typename = std::enable_if_t<std::is_base_of_v<Event, EventType>>>
    void QueueEvent(std::unique_ptr<EventType>&& event);
    
    void DispatchEvents();

private:
    EventManager() = default;
    void TriggerEvent()
    std::vector<std::unique_ptr<Event>> event_queue_;
    std::unordered_map<std::type_index, std::vector<std::unique_ptr<BaseEventHandler>>> subscribers_;
};

template<typename EventType, typename>
void EventManager::Subscribe(const EventCallbackFn<EventType>& func) 
{
    EventHandler<EventType>* handler = std::make_unique<EventHandler>(func);
    std::string name = func.target_type().name();

    auto itr = subscribers_.find(std::type_index(typeid(EventType)));
    if (itr != subscribers_.end()) {
        auto& subscribers = itr->second;
        for (auto& subscriber : subscribers) {
            if (subscriber->GetName() == name) {
                std::cerr << "This callback function has been registered" << std::endl;
                return;
            }
        }
        itr->second.push_back(std::move(handler));
    }
    else {
        subscribers_[std::type_index(typeid(EventType))].push_back(std::move(handler));
    }
}

template<typename EventType, typename>
void EventManager::Unsubscribe(const EventCallbackFn<EventType>& func) 
{
    std::string name = func.target_type().name();
    auto itr = subscribers_.find(std::type_index(typeid(EventType)));
    if (itr != subscribers_.end()) {
        auto& subscribers = itr->second;
        for (auto it = subscribers.begin(); it != subscribers.end(); ++it) {
            if (it->get()->GetName() == name) {
                subscribers.erase(it);
                return;
            }
        }
    }

    std::cerr << "this event callback function doesn't exist." << std::endl;
}

template<typename EventType, typename >
void EventManager::TriggerEvent(const EventType& evnet) 
{
    auto itr = subscribers_.find(std::type_index(typeid(EventType)));
    if (itr != subscribers_.end()) {
        for (auto& subscriber : itr->second) {
            subscriber->Exec(static_cast<Event&>(evnet));
        }
    }
}

template<typename EventType, typename>
void EventManager::QueueEvent(std::unique_ptr<EventType>&& event) 
{
       event_queue_.emplace_back(std::move(event));
}
