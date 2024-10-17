#pragma once
#include "Quark/Core/Base.h"

#include <string>

namespace quark {

struct WindowSpecification 
{
	std::string title;
	uint32_t width;
	uint32_t height;
    bool is_fullscreen;
};

// Window Interface
class Window {
public:
    Window(const WindowSpecification& spec)
        : m_title(spec.title), m_width(spec.width), m_height(spec.height), m_fullscreen(spec.is_fullscreen)
    {

    }

    virtual ~Window() = default;

    virtual void Init() = 0;
    virtual void ShutDown() = 0;

    uint32_t GetWidth() const { return m_width; }
    uint32_t GetHeight() const { return m_height; }

    virtual uint32_t GetFrambufferWidth() const = 0;
    virtual uint32_t GetFrambufferHeight() const = 0;
    virtual uint32_t GetMonitorWidth() const = 0;
    virtual uint32_t GetMonitorHeight() const = 0;
    virtual void* GetNativeWindow() = 0;

    float GetRatio() const { return (float)GetFrambufferWidth() / GetWidth(); }

protected:
    std::string m_title;

    uint32_t m_width, m_height;

    bool m_fullscreen;
};

}