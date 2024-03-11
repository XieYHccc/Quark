#pragma once

#include <format>
#include <iostream>

#include "Events/Event.h"

class WindowResizeEvent : public Event {
public:
    EVENT_TYPE(WindowResizeEvent)

    WindowResizeEvent(unsigned int width, unsigned int height)
        : width_(width)
        , height_(height)
    {
    }

    std::string ToString() const override
    {
        std::stringstream ss;
        ss << "WindowResizeEvent: " << width_ << ", " << height_;
        return ss.str();
    }

public:
    unsigned int width_ { 0 };
    unsigned int height_ { 0 };
};

class WindowCloseEvent : public Event {
public:
    EVENT_TYPE(WindowCloseEvent)
};

