#include "Platform/MacOS/WindowGLFW.h"
#include "Events/EventManager.h"
#include "Events/ApplicationEvent.h"
#include "Events/KeyEvent.h"
#include "Events/MouseEvent.h"
#include "Platform/MacOS/InputGLFW.h"

WindowGLFW::WindowGLFW()
{

}

void WindowGLFW::Init(const std::string& title, bool is_fullscreen, u32 width, u32 height)
{
    width_ = width;
    height_ = height;
    isFullscreen_ = is_fullscreen;

    // Initialize and configure glfw window 
    if (!glfwInit())
        exit(EXIT_FAILURE);

#ifdef USE_OPENGL_DRIVER
		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
		glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#endif

#ifdef USE_VULKAN_DRIVER
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
#endif

    // Create GLFW window
    monitor_ = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(monitor_);
    monitorWidth_ = mode->width;
    monitorHeight_ = mode->height;

    if (is_fullscreen) {
        width_ = mode->width;
        height_ = mode->height;
    }

    window_ = glfwCreateWindow(width_, height_, title.c_str(), is_fullscreen? monitor_ : nullptr, nullptr);
    if (!window_) {
        std::cerr << "Cannot create GLFW window.\n";
        glfwTerminate();
        exit(EXIT_FAILURE);
    }
    
#ifdef USE_OPENGL_DRIVER
    glfwMakeContextCurrent(window_);
#endif

    glfwSetWindowUserPointer(window_, reinterpret_cast<void*>(this));

#ifdef USE_OPENGL_DRIVER
    // Initialize glad: load all OpenGL function pointers
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        exit(EXIT_FAILURE);
    }
#endif

    // Set GLFW callbacks
    glfwSetWindowSizeCallback(window_, [](GLFWwindow* window, int width, int height) 
    {
        auto& owner = *(WindowGLFW*)glfwGetWindowUserPointer(window);
        owner.width_ = width;
        owner.height_ = height;
        int frame_width = 0;
        int frame_height = 0;
        glfwGetFramebufferSize(window, &frame_width, &frame_height);
        EventManager::Instance().TriggerEvent(WindowResizeEvent(frame_width, frame_height));
    });

    glfwSetWindowCloseCallback(window_, [](GLFWwindow* window)
    {
        std::unique_ptr<WindowCloseEvent> closeEvent = std::make_unique<WindowCloseEvent>();
        EventManager::Instance().QueueEvent(std::move(closeEvent));
    });
    
    glfwSetKeyCallback(window_, [](GLFWwindow* window, int key, int scancode, int action, int mods)
    {
        // Record Key status
        ((InputGLFW*)Input::Singleton())->RecordKey(key, action);

        switch (action) {
        case GLFW_PRESS: {
            EventManager::Instance().TriggerEvent(KeyPressedEvent(key, 0));
            break;
        }
        case GLFW_RELEASE: {
            EventManager::Instance().TriggerEvent(KeyReleasedEvent(key));
            break;
        }
        case GLFW_REPEAT: {
            EventManager::Instance().TriggerEvent(KeyPressedEvent(key, 1));
            break;
        }
        }
    });

    glfwSetCursorPosCallback(window_, [](GLFWwindow* window, double xpos, double ypos)
    {   
        // Record Mouse position
        ((InputGLFW*)Input::Singleton())->RecordMousePosition(xpos, ypos);

        EventManager::Instance().TriggerEvent(MouseMovedEvent((float)xpos, (float)ypos));
    });

    glfwSetScrollCallback(window_, [](GLFWwindow* window, double xOffset, double yOffset) {
        EventManager::Instance().TriggerEvent(MouseScrolledEvent((float)xOffset, (float)yOffset));
    });

    glfwSetMouseButtonCallback(window_, [](GLFWwindow* window, int button, int action, int mods) {
        // Record Mouse button status
        ((InputGLFW*)Input::Singleton())->RecordKey(button, action);

        if (action == GLFW_PRESS) {
            EventManager::Instance().TriggerEvent(MouseButtonPressedEvent(button));
        }
        else if (action == GLFW_RELEASE) {
            EventManager::Instance().TriggerEvent(MouseButtonReleasedEvent(button));   
        }
    });
}

void WindowGLFW::Finalize()
{
    glfwTerminate();
}


u32 WindowGLFW::GetFrambufferWidth() const
{   
    int width, height;
    glfwGetFramebufferSize(window_, &width, &height);

    return width;
}

u32 WindowGLFW::GetFrambufferHeight() const
{
    int width, height;
    glfwGetFramebufferSize(window_, &width, &height);

    return height;
}