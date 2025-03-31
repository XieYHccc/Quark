#include "Quark/qkpch.h"
#include "RenderContext.h"

namespace quark
{
void quark::RenderContext::SetScene(const Scene* scene)
{
	m_scene = scene;
}
void RenderContext::SetCamera(const glm::mat4& view, const glm::mat4& proj)
{
	m_camera_parameters.projection = proj;
	m_camera_parameters.view = view;
	m_camera_parameters.view_projection = proj * view;
	m_camera_parameters.inv_projection = glm::inverse(proj);
	m_camera_parameters.inv_view = glm::inverse(view);
	m_camera_parameters.inv_view_projection = glm::inverse(m_camera_parameters.view_projection);
	m_camera_parameters.camera_position = m_camera_parameters.inv_view[3].xyz;
	m_camera_parameters.camera_up = m_camera_parameters.inv_view[1].xyz;
	m_camera_parameters.camera_right = m_camera_parameters.inv_view[0].xyz;
	m_camera_parameters.camera_front = -glm::vec3(m_camera_parameters.inv_view[2].xyz);

	m_frustum.Build(m_camera_parameters.inv_view_projection);

	glm::mat2 inv_zw(m_camera_parameters.inv_projection[2].zw(), m_camera_parameters.inv_projection[3].zw());
	const auto project = [](const glm::vec2& zw) -> float {
		return -zw.x / zw.y;
	};
	m_camera_parameters.z_near = project(inv_zw * glm::vec2(0.0f, 1.0f));
	m_camera_parameters.z_far = project(inv_zw * glm::vec2(1.0f, 1.0f));
	
}
void RenderContext::SetLightingParameters(const LightingParameters* lighting)
{
	m_lighting_parameters = lighting;
}

const CameraParameters& RenderContext::GetCameraParameters() const
{
	return m_camera_parameters;
}

const LightingParameters& RenderContext::GetLightingParameters() const
{
	return *m_lighting_parameters;
}

const math::Frustum& RenderContext::GetVisibilityFrustum() const
{
	return m_frustum;
}

}