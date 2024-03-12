#include "Render/Camera.h"

#include "Application/Window/Input.h"
#include "Foundation/KeyCodes.h"

Camera Camera::global_camera = Camera(glm::vec3(0.0f, 5.0f, 20.0f));

Camera::Camera(glm::vec3 position, glm::vec3 up, float yaw, float pitch)
{
    Position = position;
    WorldUp = up;
    Yaw = yaw;
    Pitch = pitch;
    MovementSpeed = 10.f;
    MouseSensitivity = 0.1f;
    Zoom = 45.f;
    updateCameraVectors();
}

void Camera::Update(float deltaTime)
{
    if (Input::IsKeyPressed(W))
        Camera::global_camera.ProcessKeyboard(FORWARD, deltaTime);
    if (Input::IsKeyPressed(S))
        Camera::global_camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (Input::IsKeyPressed(A))
        Camera::global_camera.ProcessKeyboard(LEFT, deltaTime);
    if (Input::IsKeyPressed(D))
        Camera::global_camera.ProcessKeyboard(RIGHT, deltaTime);
}

void Camera::ProcessKeyboard(Camera_Movement direction, float deltaTime)
{
    float deltaShift = MovementSpeed * deltaTime;
    if (direction == FORWARD)
        Position += Front * deltaShift;
    if (direction == BACKWARD)
        Position -= Front * deltaShift;
    if (direction == LEFT)
        Position -= Right * deltaShift;
    if (direction == RIGHT)
        Position += Right * deltaShift;
    
    // make sure the camera stays at xz plane
    // Position.y =0.0f;
}

void Camera::ProcessMouseMovement(float xoffset, float yoffset, bool constrainPitch)
{
    xoffset *= MouseSensitivity;
    yoffset *= MouseSensitivity;

    Yaw   += xoffset;
    Pitch += yoffset;

    // make sure that when pitch is out of bounds, screen doesn't get flipped
    if (constrainPitch)
    {
        if (Pitch > 89.0f)
            Pitch = 89.0f;
        if (Pitch < -89.0f)
            Pitch = -89.0f;
    }

    // update Front, Right and Up Vectors using the updated Euler angles
    updateCameraVectors();
}

void Camera::ProcessMouseScroll(float yoffset)
{
    Zoom -= (float)yoffset;
    if (Zoom < 1.0f)
        Zoom = 1.0f;
    if (Zoom > 45.0f)
        Zoom = 45.0f;
}

void Camera::updateCameraVectors()
{
    // calculate the new Front vector
    glm::vec3 front;
    front.x = cos(glm::radians(Yaw)) * cos(glm::radians(Pitch));
    front.y = sin(glm::radians(Pitch));
    front.z = sin(glm::radians(Yaw)) * cos(glm::radians(Pitch));
    Front = glm::normalize(front);
    // also re-calculate the Right and Up vector
    Right = glm::normalize(glm::cross(Front, WorldUp));  // normalize the vectors, because their length gets closer to 0 the more you look up or down which results in slower movement.
    Up = glm::normalize(glm::cross(Right, Front));
}