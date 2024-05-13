#include "pch.h"
#include "Core/Window.h"
#include "Core/Defines.h"

#ifdef PLATFORM_APPLE
#include "Platform/MacOS//WindowGLFW.h"
#endif

Window* Window::singleton_ = nullptr;

Window* Window::Instance()
{
    return singleton_;
}

void Window::Create()
{
#ifdef PLATFORM_APPLE
    Window::singleton_ = new WindowGLFW();
#else
    XE_CORE_ASSERT_MSG(0, "XEngine currently not support this platform")
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

