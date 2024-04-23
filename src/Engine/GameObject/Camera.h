#pragma once

#include <vector>

#include <glm/glm.hpp>

// Defines several possible options for camera movement. Used as abstraction to stay away from window-system specific input methods
enum Camera_Movement {
    FORWARD,
    BACKWARD,
    LEFT,
    RIGHT
};

// An abstract camera class that processes input and calculates the corresponding Euler Angles, Vectors and Matrices for use in OpenGL
class Camera
{
public:

    // camera Attributes
    glm::vec3 position;
    glm::vec3 front;
    glm::vec3 up;
    glm::vec3 right;
    glm::vec3 worldUp;
    // euler Angles
    float yaw;
    float pitch;
    // camera options
    float movementSpeed;
    float mouseSensitivity;
    float zoom;
    float aspect;
    float zNear;
    float zFar;

public:
    // constructor with vectors
    Camera(glm::vec3 position = glm::vec3(0.f), glm::vec3 up = glm::vec3(0.f, 1.f, 0.f), float yaw = -90.f, float pitch = 0.f);

    glm::mat4 GetViewMatrix();
    glm::mat4 GetProjectionMatrix();
    void Update(float deltaTime);

private:
    void ProcessKeyboard(Camera_Movement direction, float deltaTime);
    void ProcessMouseMovement(float xoffset, float yoffset, bool constrainPitch = true);
    void ProcessMouseScroll(float yoffset);

    // calculates the front vector from the Camera's (updated) Euler Angles
    void UpdateCameraVectors();

};