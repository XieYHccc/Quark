#pragma once

#include <format>
#include <iostream>
#include "Quark/Events/Event.h"

namespace quark {

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

    std::string ToString() const override
    {
        std::stringstream ss;
        ss << "WindowCloseEvent";
        return ss.str();
    }
};

}