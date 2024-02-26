#include "Window.h"

#include <algorithm>
#include <iostream>

Window::Window(const char* title, int width, int height)
    : title_(title), width_(width), height_(height) {

    // Initialize and configure glfw window 
    if (!glfwInit())
        exit(EXIT_FAILURE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_SAMPLES, 4);

    window_ = glfwCreateWindow(width, height, title, nullptr, nullptr);

    if (!window_) {
        std::cerr << "Cannot create GLFW window.\n";
        glfwTerminate();
        exit(EXIT_FAILURE);
    }
    glfwMakeContextCurrent(window_);

    // enable v-sync
    glfwSwapInterval(1);

    // Initialize glad: load all OpenGL function pointers
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        exit(EXIT_FAILURE);
    }

    // debug: print GL and GLSL version
    std::cout << "GL     " << glGetString(GL_VERSION) << std::endl;
    std::cout << "GLSL   " << glGetString(GL_SHADING_LANGUAGE_VERSION) << std::endl;
    // call glGetError once to clear error queue
    glGetError();
    // detect highDPI framebuffer scaling and UI scaling
    int window_width, window_height, framebuffer_width, framebuffer_height;
    glfwGetWindowSize(window_, &window_width, &window_height);
    glfwGetFramebufferSize(window_, &framebuffer_width, &framebuffer_height);
    width_ = framebuffer_width;
    height_ = framebuffer_height;
    scaling_ = (float)framebuffer_width / window_width;
    if (scaling_ != 1)
        std::cout << "highDPI scaling: " << scaling_ << std::endl;

    // Register glfw callbacks
    glfwSetErrorCallback(glfw_error);
    glfwSetKeyCallback(window_, glfw_keyboard);
    glfwSetCursorPosCallback(window_, glfw_motion);
    glfwSetMouseButtonCallback(window_, glfw_mouse);
    glfwSetScrollCallback(window_, glfw_scroll);
    glfwSetFramebufferSizeCallback(window_, glfw_resize);
    glfwSetDropCallback(window_, glfw_drop);

    // tell GLFW to capture our mouse
    glfwSetInputMode(window_, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // Set window user pointer
    glfwSetWindowUserPointer(window_, reinterpret_cast<void*>(this));

    // Configure global OpenGl state
    glEnable(GL_DEPTH_TEST);

    // Init modifiers
    ctrl_pressed_ = shift_pressed_ = alt_pressed_ = false;
}

Window::~Window() {
    glfwTerminate(); // this automatically destroys remaining windows
}

int Window::run() {
    while (!glfwWindowShouldClose(window_)) {
        glfwMakeContextCurrent(window_);
        // Process inputs before rendering
        process_input(window_);
        // render the scene
        render();
        // swap buffers
        glfwSwapBuffers(window_);
        // handle events
        glfwPollEvents();
    }
    return EXIT_FAILURE;
}

void Window::glfw_error(int error, const char* description) {
    std::cerr << "error (" << error << "):" << description << std::endl;
}

void Window::glfw_keyboard(GLFWwindow* window, int key, int scancode,
    int action, int mods) {
    Window& instance = get_instance(window);
    // remember modifier status
    switch (key)
    {
    case GLFW_KEY_LEFT_CONTROL:
    case GLFW_KEY_RIGHT_CONTROL:
        instance.ctrl_pressed_ = (action != GLFW_RELEASE);
        break;
    case GLFW_KEY_LEFT_SHIFT:
    case GLFW_KEY_RIGHT_SHIFT:
        instance.shift_pressed_ = (action != GLFW_RELEASE);
        break;
    case GLFW_KEY_LEFT_ALT:
    case GLFW_KEY_RIGHT_ALT:
        instance.alt_pressed_ = (action != GLFW_RELEASE);
        break;
    }
    // send event to window
    instance.keyboard(key, scancode, action, mods);
}

void Window::keyboard(int key, int /*code*/, int action, int /*mods*/) {
    if (action != GLFW_PRESS && action != GLFW_REPEAT)
        return;

    switch (key) {
    case GLFW_KEY_ESCAPE:
    case GLFW_KEY_Q: {
        exit(0);
    }
    case GLFW_KEY_F:{
        if (!is_fullscreen())
            enter_fullscreen();
        else
            exit_fullscreen();
        break;
    }
    }
}

bool Window::is_fullscreen() const {
    return glfwGetWindowMonitor(window_) != nullptr;
}

void Window::enter_fullscreen() {
    // get monitor
    GLFWmonitor* monitor = glfwGetPrimaryMonitor();

    // get resolution
    const GLFWvidmode* mode = glfwGetVideoMode(monitor);

    // remember window position and size
    glfwGetWindowPos(window_, &backup_xpos_, &backup_ypos_);
    glfwGetWindowSize(window_, &backup_width_, &backup_height_);

    // switch to fullscreen on primary monitor
    glfwSetWindowMonitor(window_, monitor, 0, 0, mode->width, mode->height,
        GLFW_DONT_CARE);
}

void Window::exit_fullscreen() {
    glfwSetWindowMonitor(window_, nullptr, backup_xpos_, backup_ypos_,
        backup_width_, backup_height_, GLFW_DONT_CARE);
}


void Window::glfw_motion(GLFWwindow* window, double xpos, double ypos) {
    Window& instance = get_instance(window);
    // correct for highDPI scaling
    instance.motion(instance.scaling_ * xpos, instance.scaling_ * ypos);
}

void Window::glfw_mouse(GLFWwindow* window, int button, int action, int mods) {
    Window& instance = get_instance(window);
    instance.button_[button] = (action == GLFW_PRESS);
    instance.mouse(button, action, mods);
}

void Window::glfw_scroll(GLFWwindow* window, double xoffset, double yoffset) {
    Window& instance = get_instance(window);
    instance.scroll(xoffset, yoffset);
}

void Window::glfw_resize(GLFWwindow* window, int width, int height) {
    Window& instance = get_instance(window);
    instance.width_ = width;
    instance.height_ = height;
    glViewport(0, 0, width, height);
    instance.resize(width, height);
}
void Window::glfw_drop(GLFWwindow* window, int count, const char** paths) {
    Window& instance = get_instance(window);
    instance.drop(count, paths);
}

void Window::cursor_pos(double& x, double& y) const {
    glfwGetCursorPos(window_, &x, &y);
    x *= scaling_;
    y *= scaling_;
}

void Window::frame_time(float& t) const {
    t = static_cast<float>(glfwGetTime());
}


