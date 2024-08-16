#pragma once
#include "Quark/Core/KeyMouseCodes.h"
#include "Quark/Events/Event.h"

namespace quark {

class KeyPressedEvent : public Event {
public:
    KeyPressedEvent(KeyCode keyCode, int repeatCount) : key(keyCode), repeatCount(repeatCount) {}

    std::string ToString() const override
    {
        std::stringstream ss;
        ss << "KeyPressedEvent: " << key << "( " << repeatCount<< " repeats)";
        return ss.str();
    }

    EVENT_TYPE("KeyPressedEvent")
    
public:
    KeyCode key;
    int repeatCount;
};

class KeyReleasedEvent : public Event {
public:
    KeyReleasedEvent(KeyCode keyCode) : key(keyCode) {}

    std::string ToString() const override
    {
        std::stringstream ss;
        ss << "KeyReleasedEvent: " << key;
        return ss.str();
    }

    EVENT_TYPE("KeyReleasedEvent")
public:
    KeyCode key;
};

class KeyTypedEvent : public Event {
public:

    KeyTypedEvent(KeyCode keyCode) : key(keyCode) {}

    std::string ToString() const override
    {
        std::stringstream ss;
        ss << "KeyTdypedEvent: " << key;
        return ss.str();
    }

    EVENT_TYPE("KeyTypedEvent")
public:
    KeyCode key;
};

}