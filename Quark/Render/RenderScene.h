#pragma once
#include "Quark/Render/RenderTypes.h"
#include "Quark/Core/Math/Frustum.h"

namespace quark
{
	class RenderScene;
	struct Visibility
	{
		Ref<RenderScene> render_scene;

		// visible objects (updated per frame)
		std::vector<uint32_t> main_camera_visible_object_indexes;
		std::vector<uint32_t> directional_light_visible_object_indexes;
		std::vector<uint32_t> point_lights_visible_object_indexes;

		UniformBufferObject_Camera camera_ubo_data;
		math::Frustum frustum;
	};

	class RenderScene
	{
	public:
		// lights TODO: rewrite after we have light components
		glm::vec4 ambientColor;
		glm::vec4 sunlightDirection; // w for sun power
		glm::vec4 sunlightColor;

		// render entities
		std::vector<RenderObject> render_objects;

		// ubo data
		Ref<rhi::Buffer> per_frame_ubo_scene;
		UniformBufferObject_Scene ubo_data_scene;

		// cache
		std::unordered_map<uint64_t, size_t> render_object_to_offset;
		std::unordered_map<uint64_t, uint64_t> render_object_to_entity;

		Visibility main_camera_visibility;

		RenderScene();
		
		void DeleteRenderObjectsByEntityID(uint64_t entity_id);
		void AddOrUpdateRenderObject(const RenderObject& entity, uint64_t entity_id);

		void UpdateVisibility(Visibility& out_vis, const UniformBufferObject_Camera& cameraData);

	};
}