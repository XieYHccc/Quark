#pragma once

#include "Foundation/KeyCodes.h"
#include "Events/Event.h"

class KeyPressedEvent : public Event {
public:
    KeyPressedEvent(int keyCode, int repeatCount) : key(keyCode), repeatCount(repeatCount) {}

    std::string ToString() const override
    {
        std::stringstream ss;
        ss << "KeyPressedEvent: " << key << "( " << repeatCount<< " repeats)";
        return ss.str();
    }

    EVENT_TYPE("KeyPressedEvent")
    
public:
    Keycode key;
    int repeatCount;
};

class KeyReleasedEvent : public Event {
public:
    KeyReleasedEvent(int keyCode) : key(keyCode) {}

    std::string ToString() const override
    {
        std::stringstream ss;
        ss << "KeyReleasedEvent: " << key;
        return ss.str();
    }

    EVENT_TYPE("KeyReleasedEvent")
public:
    Keycode key;
};

class KeyTypedEvent : public Event {
public:

    KeyTypedEvent(int keyCode) : key(keyCode) {}

    std::string ToString() const override
    {
        std::stringstream ss;
        ss << "KeyTdypedEvent: " << key;
        return ss.str();
    }

    EVENT_TYPE("KeyTypedEvent")
public:
    Keycode key;
};
