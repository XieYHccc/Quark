#pragma once
#include "Core/Base.h"

// Window Interface
class Window {
public:
    static void Create();
    static void Destroy();
    static Window* Instance();
private:
    static Window* singleton_;

public:

    virtual void Init(const std::string& title, bool is_fullscreen, u32 width, u32 height) = 0;
    virtual void Finalize() = 0;

    virtual u32 GetWidth() const = 0;
	virtual u32 GetHeight() const = 0;
    virtual u32 GetFrambufferWidth() const = 0;
    virtual u32 GetFrambufferHeight() const = 0;
    virtual u32 GetMonitorWidth() const = 0;
    virtual u32 GetMonitorHeight() const = 0;
    float GetRatio() const { return (float)GetFrambufferWidth() / GetWidth(); }

    virtual void* GetNativeWindow() = 0;

protected:
    virtual ~Window() = default;
};