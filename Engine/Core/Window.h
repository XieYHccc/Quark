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
    // update per frame
    virtual void Update() = 0;

    virtual uint32_t GetWidth() const = 0;
	virtual uint32_t GetHeight() const = 0;
    //TODO: support to query framebuffer size


    virtual void* GetNativeWindow() = 0;

protected:
    virtual ~Window() = default;
};