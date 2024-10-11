#pragma once
#include <glm/glm.hpp>
#include <Quark/Core/TimeStep.h>

namespace quark {
class EditorCamera {
public:
	float fov = 45.0f;
	float viewportWidth = 1280;
	float viewportHeight = 720;
	float nearClip = 0.1f;
	float farClip = 100.0f;
	float moveSpeed = 20.0f;
	float rotationSpeed = 0.3f;
	
	EditorCamera(float degreeFov, float viewportWidth, float viewportHeight, float nearClip, float farClip);

	void OnUpdate(TimeStep timestep);

	void Init();

	glm::mat4 GetViewMatrix() const;
	glm::mat4 GetProjectionMatrix() const;

	glm::quat GetRotation() const;
	glm::vec3& GetPosition() { return m_Position; }

	glm::vec3 GetFowardDirection() const;
	glm::vec3 GetRightDirection() const;
	glm::vec3 GetUpDirection() const;

	void SetViewportSize(float width, float height) { viewportWidth = width; viewportHeight = height; }
	void SetPosition(const glm::vec3& position) { m_Position = position; }
	void SetYaw(float yaw) { m_Yaw = yaw; } // in radians
	void SetPitch(float pitch) { m_Pitch = pitch; } // in radians

private:
	std::pair<float, float> PanSpeed() const;

	void MousePan(const glm::vec2& delta);
	void MouseRotate(const glm::vec2& delta);
	void MouseZoom(float delta);
private:
	glm::vec3 m_Position;
	float m_Pitch, m_Yaw;	// in radians

	glm::vec2 m_LastMousePosition;
	glm::vec3 m_FocalPoint;
};
}