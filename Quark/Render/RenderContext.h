#pragma once
#include "Quark/Render/RenderParameters.h"
#include "Quark/Scene/Scene.h"
#include "Quark/Core/Math/Frustum.h"

namespace quark 
{

// RenderContext is a class that holds global render data for a frame
// that is shared between all renderables
class RenderContext 
{
public:
    void SetScene(const Scene* scene);
    void SetCamera(const glm::mat4& view, const glm::mat4& proj);
    void SetLightingParameters(const LightingParameters* lighting);
    
    const CameraParameters& GetCameraParameters() const;
    const LightingParameters& GetLightingParameters() const;
    const math::Frustum& GetVisibilityFrustum() const;
    
private:
    const Scene* m_scene;
    const LightingParameters* m_lighting_parameters;
    CameraParameters m_camera_parameters;
    math::Frustum m_frustum;

};
}