#pragma once

#include "Core/KeyMouseCodes.h"
#include "Events/Event.h"

class MouseMovedEvent : public Event {
public:
    MouseMovedEvent(float x, float y) : mouseX(x), mouseY(y) {}

    std::string ToString() const override
    {
        std::stringstream ss;
        ss << "MouseMovedEvent: " << mouseX << ", " << mouseY;
        return ss.str();
    }

    EVENT_TYPE("MouseMovedEvent")

public:
    float mouseX;
    float mouseY;
};

class MouseScrolledEvent : public Event {
public:

    MouseScrolledEvent(float xOffset_, float yOffset_) : xOffset(xOffset_), yOffset(yOffset_) {}

    std::string ToString() const override
    {
        std::stringstream ss;
        ss << "MouseMovedEvent: " << xOffset << ", " << yOffset;
        return ss.str();
    }

    EVENT_TYPE("MouseScrolledEvent")

public:
    float xOffset;
    float yOffset;
};

class MouseButtonPressedEvent : public Event {
public:
    MouseButtonPressedEvent(MouseCode button) : button(button) {}

    std::string ToString() const override
    {
        std::stringstream ss;
        ss << "MouseButtonPressedEvent: " << button;
        return ss.str();
    }

    EVENT_TYPE("MouseButtonPressedEvent")

public:
    MouseCode button;
};

class MouseButtonReleasedEvent : public Event {
public:
    MouseButtonReleasedEvent(int button) : button(button) {}

    std::string ToString() const override
    {   
        std::stringstream ss;
        ss << "MouseButtonReleasedEvent: " << button;
        return ss.str();
    }

    EVENT_TYPE("MouseButtonReleasedEvent")

public:
    MouseCode button;
};