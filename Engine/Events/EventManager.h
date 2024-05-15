#pragma once

#include <iostream>

#include "EventHandler.h"

class EventManager {
public:
    using EventType = Event::EventType;

    static EventManager& Instance() {
        static EventManager instance;
        return instance;
    }
    
    void Init() {};
    void Finalize() {}

public:
    template<typename T, typename = std::enable_if_t<std::is_base_of_v<Event, T>>>
    void Subscribe(const EventCallbackFn<T>& func);

    template<typename T, typename = std::enable_if_t<std::is_base_of_v<Event, T>>>
    void Unsubscribe(const EventCallbackFn<T>& func);

    template<typename T, typename = std::enable_if_t<std::is_base_of_v<Event, T>>>
    void QueueEvent(std::unique_ptr<T>&& event);

    void TriggerEvent(const Event& evnet);

    void DispatchEvents();

private:
    EventManager() = default;

    std::vector<std::unique_ptr<Event>> event_queue_;
    std::unordered_map<EventType, std::vector<std::unique_ptr<BaseEventHandler>>> subscribers_;
};

template<typename T, typename>
void EventManager::Subscribe(const EventCallbackFn<T>& func) 
{
    auto handler = std::make_unique<EventHandler<T>>(func);
    std::string name = func.target_type().name();

    auto itr = subscribers_.find(T::GetStaticEventType());
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
        subscribers_[T::GetStaticEventType()].push_back(std::move(handler));
    }
}

template<typename T, typename>
void EventManager::Unsubscribe(const EventCallbackFn<T>& func) 
{
    std::string name = func.target_type().name();
    auto itr = subscribers_.find(T::GetStaticEventType());
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

template<typename T, typename>
void EventManager::QueueEvent(std::unique_ptr<T>&& event) 
{
    event_queue_.push_back(std::move(event));
}