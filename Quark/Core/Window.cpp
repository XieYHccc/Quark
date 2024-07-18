#include "pch.h"
#include "Core/Window.h"

#ifdef QK_PLATFORM_MACOS
#include "Platform/MacOS/WindowGLFW.h"
#endif

Window* Window::singleton_ = nullptr;

Window* Window::Instance()
{
    return singleton_;
}

void Window::Create()
{
#ifdef QK_PLATFORM_MACOS
    Window::singleton_ = new WindowGLFW();
#else
    #error "Platform doesn't support window"
#endif

}

void Window::Destroy()
{
    if(singleton_ != nullptr)
    {
        singleton_->Finalize();
        delete singleton_;
    }
}

