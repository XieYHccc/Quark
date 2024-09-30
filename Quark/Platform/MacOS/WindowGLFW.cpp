#include "Quark/qkpch.h"
#include "Quark/Platform/MacOS/WindowGLFW.h"
#include "Quark/Events/EventManager.h"
#include "Quark/Events/ApplicationEvent.h"
#include "Quark/Events/KeyEvent.h"
#include "Quark/Events/MouseEvent.h"
#include "Quark/Platform/MacOS/InputGLFW.h"

namespace quark {

WindowGLFW::WindowGLFW(const WindowSpecification& spec)
    : Window(spec)
{

}

void WindowGLFW::Init()
{

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
    m_glfwMonitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(m_glfwMonitor);
    m_monitorWidth = mode->width;
    m_monitorHeight = mode->height;

    if (m_fullscreen) 
    {
        m_width = mode->width;
        m_height = mode->height;
    }

    m_glfwWindow = glfwCreateWindow(m_width, m_height, m_title.c_str(), m_fullscreen? m_glfwMonitor : nullptr, nullptr);
    if (!m_glfwWindow) 
    {
        QK_CORE_LOGF_TAG("Core", "Cannot create GLFW window.");
        glfwTerminate();
        exit(EXIT_FAILURE);
    }
    
#ifdef USE_OPENGL_DRIVER
    glfwMakeContextCurrent(m_glfwWindow);
#endif

    glfwSetWindowUserPointer(m_glfwWindow, reinterpret_cast<void*>(this));

#ifdef USE_OPENGL_DRIVER
    // Initialize glad: load all OpenGL function pointers
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        exit(EXIT_FAILURE);
    }
#endif

    // Set GLFW callbacks
    glfwSetWindowSizeCallback(m_glfwWindow, [](GLFWwindow* window, int width, int height) 
    {
        auto& owner = *(WindowGLFW*)glfwGetWindowUserPointer(window);
        owner.m_width = width;
        owner.m_height = height;

        int frame_width = 0;
        int frame_height = 0;
        glfwGetFramebufferSize(window, &frame_width, &frame_height);
        EventManager::Get().TriggerEvent(WindowResizeEvent(frame_width, frame_height));
    });

    glfwSetWindowCloseCallback(m_glfwWindow, [](GLFWwindow* window)
    {
        std::unique_ptr<WindowCloseEvent> closeEvent = std::make_unique<WindowCloseEvent>();
        EventManager::Get().QueueEvent(std::move(closeEvent));
    });
    
    glfwSetKeyCallback(m_glfwWindow, [](GLFWwindow* window, int key, int scancode, int action, int mods)
    {
        // Record Key status
        ((InputGLFW*)Input::Get())->RecordKey(key, action);

        switch (action) {
        case GLFW_PRESS: {
            EventManager::Get().TriggerEvent(KeyPressedEvent(key, 0));
            break;
        }
        case GLFW_RELEASE: {
            EventManager::Get().TriggerEvent(KeyReleasedEvent(key));
            break;
        }
        case GLFW_REPEAT: {
            EventManager::Get().TriggerEvent(KeyPressedEvent(key, 1));
            break;
        }
        }
    });

    glfwSetCursorPosCallback(m_glfwWindow, [](GLFWwindow* window, double xpos, double ypos)
    {   
        // Record Mouse position
        ((InputGLFW*)Input::Get())->RecordMousePosition((float)xpos, (float)ypos);

        EventManager::Get().TriggerEvent(MouseMovedEvent((float)xpos, (float)ypos));
    });

    glfwSetScrollCallback(m_glfwWindow, [](GLFWwindow* window, double xOffset, double yOffset) {
        EventManager::Get().TriggerEvent(MouseScrolledEvent((float)xOffset, (float)yOffset));
    });

    glfwSetMouseButtonCallback(m_glfwWindow, [](GLFWwindow* window, int button, int action, int mods) {
        // Record Mouse button status
        ((InputGLFW*)Input::Get())->RecordKey(button, action);

        if (action == GLFW_PRESS) {
            EventManager::Get().TriggerEvent(MouseButtonPressedEvent(button));
        }
        else if (action == GLFW_RELEASE) {
            EventManager::Get().TriggerEvent(MouseButtonReleasedEvent(button));   
        }
    });
}

void WindowGLFW::ShutDown()
{
    glfwTerminate();
}


u32 WindowGLFW::GetFrambufferWidth() const
{   
    int width, height;
    glfwGetFramebufferSize(m_glfwWindow, &width, &height);

    return width;
}

u32 WindowGLFW::GetFrambufferHeight() const
{
    int width, height;
    glfwGetFramebufferSize(m_glfwWindow, &width, &height);

    return height;
}

}