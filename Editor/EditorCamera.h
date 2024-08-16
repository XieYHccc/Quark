#pragma once
#include <glm/glm.hpp>
#include <Quark/Core/Timestep.h>

#include "Editor/UI/SceneViewPort.h"

namespace quark {
class EditorCamera {
public:
	float fov = 45.0f;
	float aspectRatio = 1.0f;
	float nearClip = 0.1f;
	float farClip = 256.0f;
	float moveSpeed = 25.0f;
	float rotationSpeed = 0.3f;
	
	EditorCamera(float degreeFov, float aspectRatio, float nearClip, float farClip);

	void OnUpdate(TimeStep timestep);
	void OnViewPortHovered(const SceneViewPortTouchedEvent& event); //Kinda ugly here, but it's fine for now

	void Init();

	glm::mat4 GetViewMatrix() const;
	glm::mat4 GetProjectionMatrix() const;

	glm::quat GetRotation() const;
	glm::vec3& GetPosition() { return m_Position; }

	glm::vec3 GetFowardDirection() const;
	glm::vec3 GetRightDirection() const;
	glm::vec3 GetUpDirection() const;


private:
	glm::vec3 m_Position;
	float m_Pitch, m_Yaw;	// in radians

	glm::vec2 m_LastMousePosition;
	bool m_IsFirstMouse;
	bool m_IsViewPortHovered;
};
}