#pragma once

#include <vector>
#include <utility>
#include <array>
#include <string>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

// A window provided by GLFW
class Window
{
public:
    Window(const char* title, int width, int height);
    virtual ~Window();
    int run();

protected:
    // this function is called when the scene has to be rendered
    virtual void render() = 0;
    virtual void keyboard(int /*key*/, int /*code*/, int /*action*/, int /*mods*/);
    virtual void mouse(int /*button*/, int /*action*/, int /*mods*/) {}
    virtual void motion(double /*xpos*/, double /*ypos*/) {}
    virtual void scroll(double /*xoffset*/, double /*yoffset*/) {}
    virtual void resize(int /*width*/, int /*height*/) {}
    virtual void drop(int /*count*/, const char** /*paths*/) {}
    virtual void process_input(GLFWwindow* window) {}

    int width() const { return width_; }
    int height() const { return height_; }

    void cursor_pos(double& x, double& y) const;   // get position of mouse cursor
    void frame_time(float& t) const;               // get time of current frame
    bool left_mouse_pressed() const { return button_[GLFW_MOUSE_BUTTON_LEFT];  }
    bool right_mouse_pressed() const { return button_[GLFW_MOUSE_BUTTON_RIGHT]; }
    bool middle_mouse_pressed() const { return button_[GLFW_MOUSE_BUTTON_MIDDLE]; }
    bool ctrl_pressed() const { return ctrl_pressed_; }
    bool alt_pressed() const { return alt_pressed_; }
    bool shift_pressed() const { return shift_pressed_; }

private:
    static void glfw_error(int error, const char* description);
    static void glfw_keyboard(GLFWwindow* window, int key, int scancode, int action, int mods);
    static void glfw_mouse(GLFWwindow* window, int button, int action, int mods);
    static void glfw_motion(GLFWwindow* window, double xpos, double ypos);
    static void glfw_scroll(GLFWwindow* window, double xoffset, double yoffset);
    static void glfw_resize(GLFWwindow* window, int width, int height);
    static void glfw_drop(GLFWwindow* window, int count, const char** paths);

    // Get current instance to call non-static methods
    static Window& get_instance(GLFWwindow* window) { return *static_cast<Window*>(glfwGetWindowUserPointer(window)); }

private:
    GLFWwindow* window_;
    std::string title_;
    int width_, height_;

    // highDPI scaling
    float scaling_{ 1.0 };

    // which mouse buttons and modifier keys are pressed down
    std::array<bool, 7> button_;
    bool ctrl_pressed_;
    bool alt_pressed_;
    bool shift_pressed_;

    // fullscreen-related backups
    int backup_xpos_;
    int backup_ypos_;
    int backup_width_;
    int backup_height_;
    bool is_fullscreen() const;
    void enter_fullscreen();
    void exit_fullscreen();
};