#include "Application/Window/Window.h"

#include <iostream>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "Events/EventManager.h"
#include "Events/ApplicationEvent.h"
#include "Events/KeyEvent.h"
#include "Events/MouseEvent.h"

Window::~Window()
{
    Finalize();
}

void Window::Initialize(const WindowProps& props) {
    // set window properties
    props_ = props;

    // Initialize and configure glfw window 
    if (!glfwInit())
        exit(EXIT_FAILURE);

#ifdef GRAPHIC_API_OPENGL
		windowName = "XEngine <OpenGL " + 
			std::to_string(3) +"." + 
			std::to_string(3) + ">";

		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
		glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#endif

#ifdef GRAPHIC_API_VULKAN
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
#endif

    // Create GLFW window
    monitor_ = glfwGetPrimaryMonitor();
    if (props_.is_fullscreen) 
    {
        const GLFWvidmode* mode = glfwGetVideoMode(monitor_);
        props_.width = mode->width;
        props_.height = mode->height;
    }

    window_ = glfwCreateWindow(props_.width, props_.height, props_.title.c_str(), props_.is_fullscreen? monitor_ : nullptr, nullptr);
    if (!window_) {
        std::cerr << "Cannot create GLFW window.\n";
        glfwTerminate();
        exit(EXIT_FAILURE);
    }
    
#ifdef GRAPHIC_API_OPENGL
    glfwMakeContextCurrent(window_);
#endif

    glfwSetWindowUserPointer(window_, reinterpret_cast<void*>(&props_));
    SetVSync(props_.is_vsync);

#ifdef GRAPHIC_API_OPENGL
    // Initialize glad: load all OpenGL function pointers
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        exit(EXIT_FAILURE);
    }
#endif

    // Set GLFW callbacks
    glfwSetWindowSizeCallback(window_, [](GLFWwindow* window, int width, int height) 
    {
        WindowProps& props = *(WindowProps*)glfwGetWindowUserPointer(window);
        props.width = width;
        props.height = height;
        EventManager::Instance().TriggerEvent(WindowResizeEvent(width, height));
    });

    glfwSetWindowCloseCallback(window_, [](GLFWwindow* window)
    {
        std::unique_ptr<WindowCloseEvent> closeEvent = std::make_unique<WindowCloseEvent>();
        EventManager::Instance().QueueEvent(std::move(closeEvent));
    });
    
    glfwSetKeyCallback(window_, [](GLFWwindow* window, int key, int scancode, int action, int mods)
    {
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
        EventManager::Instance().TriggerEvent(MouseMovedEvent((float)xpos, (float)ypos));
    });

    glfwSetScrollCallback(window_, [](GLFWwindow* window, double xOffset, double yOffset) {
        EventManager::Instance().TriggerEvent(MouseScrolledEvent((float)xOffset, (float)yOffset));
        });

    glfwSetCursorPosCallback(window_, [](GLFWwindow* window, double xPos, double yPos) {
        EventManager::Instance().TriggerEvent(MouseMovedEvent((float)xPos, (float)yPos));
        });

#ifdef GRAPHIC_API_OPENGL
    // Configure global OpenGl state
    glEnable(GL_DEPTH_TEST);
#endif

}

void Window::Finalize()
{
    glfwTerminate();
}

void Window::SetVSync(bool enabled)
{
    if (enabled)
        glfwSwapInterval(1);
    else
        glfwSwapInterval(0);

    props_.is_vsync = enabled;
}

float Window::GetFrameTime() const
{
    return static_cast<float>(glfwGetTime());
}

void Window::Update()
{
    // swap buffers
    glfwSwapBuffers(window_);
    // handle events
    glfwPollEvents();
}