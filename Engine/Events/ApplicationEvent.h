#pragma once

#include <format>
#include <iostream>

#include "Events/Event.h"

class WindowResizeEvent : public Event {
public:
    EVENT_TYPE("WindowResizeEvent")

    WindowResizeEvent(unsigned int width, unsigned int height)
        : width(width)
        , height(height)
    {
    }

    std::string ToString() const override
    {
        std::stringstream ss;
        ss << "WindowResizeEvent: " << width << ", " << height;
        return ss.str();
    }

public:
    unsigned int width;
    unsigned int height;
};

class WindowCloseEvent : public Event {
public:
    EVENT_TYPE("WindowCloseEvent")
};

