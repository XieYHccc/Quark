#include "GameObject/Camera.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/transform.hpp>
#include <glm/gtx/quaternion.hpp>

#include "Application/Window/Input.h"
#include "Foundation/KeyCodes.h"

Camera::Camera(glm::vec3 position, glm::vec3 up, float yaw, float pitch)
{
    this->position = position;
    this->worldUp = up;
    this->yaw = yaw;
    this->pitch = pitch;
    this->movementSpeed = 10.f;
    this->mouseSensitivity = 0.1f;
    this->zoom = 45.f;
    UpdateCameraVectors();
}

void Camera::Update(float deltaTime)
{
    if (Input::IsKeyPressed(W))
        ProcessKeyboard(FORWARD, deltaTime);
    if (Input::IsKeyPressed(S))
        ProcessKeyboard(BACKWARD, deltaTime);
    if (Input::IsKeyPressed(A))
        ProcessKeyboard(LEFT, deltaTime);
    if (Input::IsKeyPressed(D))
        ProcessKeyboard(RIGHT, deltaTime);

    MousePosition pos = Input::GetMousePosition();
    if (Input::first_mouse_) {
        Input::last_position_ = pos;
        Input::first_mouse_ = false;
    }
    float xoffset = pos.x_pos - Input::last_position_.x_pos;
    float yoffset = Input::last_position_.y_pos - pos.y_pos; // reversed since y-coordinates go from bottom to top
    ProcessMouseMovement(xoffset, yoffset);

    Input::last_position_ = pos;
}

void Camera::ProcessKeyboard(Camera_Movement direction, float deltaTime)
{
    float deltaShift = movementSpeed * deltaTime;
    if (direction == FORWARD)
        position += front * deltaShift;
    if (direction == BACKWARD)
        position -= front * deltaShift;
    if (direction == LEFT)
        position -= right * deltaShift;
    if (direction == RIGHT)
        position += right * deltaShift;
    
    // make sure the camera stays at xz plane
    // Position.y =0.0f;
}

void Camera::ProcessMouseMovement(float xoffset, float yoffset, bool constrainPitch)
{
    xoffset *= mouseSensitivity;
    yoffset *= mouseSensitivity;

    yaw   += xoffset;
    pitch += yoffset;

    // make sure that when pitch is out of bounds, screen doesn't get flipped
    if (constrainPitch)
    {
        if (pitch > 89.0f)
            pitch = 89.0f;
        if (pitch < -89.0f)
            pitch = -89.0f;
    }

    // update Front, Right and Up Vectors using the updated Euler angles
    UpdateCameraVectors();
}

void Camera::ProcessMouseScroll(float yoffset)
{
    zoom -= (float)yoffset;
    if (zoom < 1.0f)
        zoom = 1.0f;
    if (zoom > 45.0f)
        zoom = 45.0f;
}

void Camera::UpdateCameraVectors()
{
    // calculate the new Front vector
    front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    front.y = sin(glm::radians(pitch));
    front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    front = glm::normalize(front);
    // also re-calculate the Right and Up vector
    right = glm::normalize(glm::cross(front, worldUp));  // normalize the vectors, because their length gets closer to 0 
    up = glm::normalize(glm::cross(right, front));
}

glm::mat4 Camera::GetProjectionMatrix()
{
#ifdef GLM_FORCE_DEPTH_ZERO_TO_ONE
    return glm::perspective(glm::radians(70.f), aspect, zNear, zFar);
#else
    return glm::perspective(glm::radians(70.f), aspect, zNear, zFar);
#endif

}

glm::mat4 Camera::GetViewMatrix()
{
    return glm::lookAt(position, position + front, up);
    // glm::quat pitchRotation = glm::angleAxis(Pitch, glm::vec3 { 1.f, 0.f, 0.f });
    // glm::quat yawRotation = glm::angleAxis(Yaw, glm::vec3 { 0.f, -1.f, 0.f });
    // glm::mat4 cameraTranslation = glm::translate(glm::mat4(1.f), Position);
    // return glm::inverse(cameraTranslation* glm::toMat4(yawRotation) * glm::toMat4(pitchRotation));
}