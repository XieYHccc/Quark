#include "Editor/EditorCamera.h"

#include <algorithm>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include <Quark/Core/Input.h>
#include <Quark/Events/EventManager.h>

namespace quark {

EditorCamera::EditorCamera(float _degreeFov, float _viewportWidth, float _viewportHeight, float _nearClip, float _farClip)
	: fov(_degreeFov), viewportWidth(_viewportWidth), viewportHeight(_viewportHeight), nearClip(_nearClip), farClip(_farClip), m_FocalPoint(0.f), m_Position(0.f)
{
	Init();
}

void EditorCamera::Init()
{
	m_Position = glm::vec3(0.0f, 0.0f, 5.0f);
	m_FocalPoint = glm::vec3(0.0f, 0.0f, 0.0f);
	m_Pitch = 0.0f;
	m_Yaw = 0.0f;
	m_LastMousePosition = {0, 0};

}

void EditorCamera::OnUpdate(TimeStep timestep)
{
	glm::vec2 mousePos = { Input::Get()->GetMousePosition().x_pos, Input::Get()->GetMousePosition().y_pos };
	glm::vec2 delta = mousePos - m_LastMousePosition;
	m_LastMousePosition = mousePos;

	if (Input::Get()->IsMousePressed(Mouse::ButtonLeft, true))
		MouseRotate(delta);
	else if (Input::Get()->IsMousePressed(Mouse::ButtonMiddle, true))
		MousePan(delta);
	else if (Input::Get()->IsMousePressed(Mouse::ButtonRight, true))
		MouseZoom(delta.y);

	// Process key input
	glm::vec3 move{ 0.f };
	if (Input::Get()->IsKeyPressed(Key::W, true))
		move.z = -1;
	if (Input::Get()->IsKeyPressed(Key::S, true))
		move.z = 1;
	if (Input::Get()->IsKeyPressed(Key::A, true))
		move.x = -1;
	if (Input::Get()->IsKeyPressed(Key::D, true))
		move.x = 1;

	move = move * moveSpeed * timestep.GetSeconds();
	m_Position += glm::rotate(GetRotation(), move);
}

glm::mat4 EditorCamera::GetViewMatrix() const
{
	return glm::lookAt(m_Position, m_Position + GetFowardDirection(), GetUpDirection());
}

glm::mat4 EditorCamera::GetProjectionMatrix() const
{
	return glm::perspective(glm::radians(fov), (viewportWidth / viewportHeight), nearClip, farClip);
}

glm::quat EditorCamera::GetRotation() const
{
	return glm::quat(glm::vec3(m_Pitch, m_Yaw, 0.f));
}

glm::vec3 EditorCamera::GetFowardDirection() const
{
	return glm::rotate(GetRotation(), glm::vec3(0.f, 0.f, -1.f));
}

glm::vec3 EditorCamera::GetRightDirection() const
{
	return glm::rotate(GetRotation(), glm::vec3(1.f, 0.f, 0.f));
}

glm::vec3 EditorCamera::GetUpDirection() const
{
	return glm::rotate(GetRotation(), glm::vec3(0.f, 1.f, 0.f));
}

std::pair<float, float> EditorCamera::PanSpeed() const
{
	float x = std::min(viewportWidth / 1000.0f, 2.4f); // max = 2.4f
	float xFactor = 0.0366f * (x * x) - 0.1778f * x + 0.3021f;

	float y = std::min(viewportHeight / 1000.0f, 2.4f); // max = 2.4f
	float yFactor = 0.0366f * (y * y) - 0.1778f * y + 0.3021f;

	return { xFactor, yFactor };
}

void EditorCamera::MousePan(const glm::vec2& delta)
{
	auto [xspeed, yspeed] = PanSpeed();
	m_Position += GetRightDirection() * delta.x * xspeed;
	m_Position -= GetUpDirection() * delta.y * yspeed;
}

void EditorCamera::MouseRotate(const glm::vec2& delta)
{
	m_Pitch -= (glm::radians(delta.y) * rotationSpeed);
	m_Yaw -= (glm::radians(delta.x) * rotationSpeed);
	m_Pitch = std::clamp(m_Pitch, -1.5f, 1.5f);
}

void EditorCamera::MouseZoom(float delta)
{
	m_Position += GetFowardDirection() * delta * 0.1f;
}

} // namespace quark