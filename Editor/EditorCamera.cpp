#include "Editor/EditorCamera.h"

#include <algorithm>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include <Quark/Core/Input.h>
#include <Quark/Events/EventManager.h>

namespace quark {

EditorCamera::EditorCamera(float degreeFov, float aspectRatio, float nearClip, float farClip)
	: fov(degreeFov), aspectRatio(aspectRatio), nearClip(nearClip), farClip(farClip)
{
	Init();

	EventManager::Instance().Subscribe<SceneViewPortTouchedEvent>([&](const SceneViewPortTouchedEvent& e) {
		OnViewPortHovered(e);
	});
}

void EditorCamera::Init()
{
	m_Position = glm::vec3(0.0f, 0.0f, 5.0f);
	m_Pitch = 0.0f;
	m_Yaw = 0.0f;
	m_IsFirstMouse = true;
	m_LastMousePosition = {0, 0};
	m_IsViewPortHovered = false;

}

void EditorCamera::OnUpdate(TimeStep timestep)
{
	glm::vec2 mousePos = { Input::Get()->GetMousePosition().x_pos, Input::Get()->GetMousePosition().y_pos };
	if (m_IsFirstMouse)
	{
		m_LastMousePosition = mousePos;
		m_IsFirstMouse = false;
	}

	// Process mouse movement
	float xoffset = mousePos.x - m_LastMousePosition.x;
	float yoffset = mousePos.y - m_LastMousePosition.y;

	if (!m_IsViewPortHovered) 
	{ 
		m_IsFirstMouse = true;
		return;
	}

	if (Input::Get()->IsMousePressed(Mouse::ButtonLeft, true))
	{
		// Process mouse movement
		m_Pitch -= (glm::radians(yoffset) * rotationSpeed);
		m_Yaw -= (glm::radians(xoffset) * rotationSpeed);
		m_Pitch = std::clamp(m_Pitch, -1.5f, 1.5f);		// make sure that when pitch is out of bounds, screen doesn't get flipped

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
	
	m_IsViewPortHovered = false;
	m_LastMousePosition = mousePos;
}

void EditorCamera::OnViewPortHovered(const SceneViewPortTouchedEvent& event)
{
	m_IsViewPortHovered = true;
}

glm::mat4 EditorCamera::GetViewMatrix() const
{
	return glm::lookAt(m_Position, m_Position + GetFowardDirection(), GetUpDirection());
}

glm::mat4 EditorCamera::GetProjectionMatrix() const
{
	return glm::perspective(glm::radians(fov), aspectRatio, nearClip, farClip);
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

} // namespace quark