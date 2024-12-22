#include "Quark/qkpch.h"
#include "Quark/Render/RenderScene.h"

namespace quark 
{
    RenderScene::RenderScene()
    {
        ambientColor = glm::vec4(.1f);
        sunlightDirection = glm::vec4(1.f);
        sunlightColor = glm::vec4(0, 1, 0.5, 1.f);

        ubo_data_scene.ambientColor = ambientColor;
        ubo_data_scene.sunlightDirection = sunlightDirection;
        ubo_data_scene.sunlightColor = sunlightColor;
    }
    
    void RenderScene::DeleteRenderObjectsByEntityID(uint64_t entity_id)
    {
        std::vector<uint64_t> to_delete_entity_ids;
        for (auto it = render_object_to_entity.begin(); it != render_object_to_entity.end();)
        {
            if (it->second == entity_id)
            {
                uint64_t render_entity_id = it->first;
                size_t offset = render_object_to_offset[render_entity_id];
                render_objects[offset] = render_objects.back();
                render_object_to_offset[render_objects.back().id] = offset;
                render_objects.pop_back();
                render_object_to_offset.erase(render_entity_id);
                it = render_object_to_entity.erase(it);
            }
            else
                ++it;
        }
    }

    void RenderScene::AddOrUpdateRenderObject(const RenderObject& obj, uint64_t entity_id)
    {
        auto find = render_object_to_offset.find(obj.id);
        if (find != render_object_to_offset.end())
        {
            render_objects[find->second] = obj;
        }
        else
        {
            render_objects.push_back(obj);
            render_object_to_offset[obj.id] = render_objects.size() - 1;
            render_object_to_entity[obj.id] = entity_id;
        }
    }

    void RenderScene::UpdateVisibility(Visibility& out_vis, const UniformBufferData_Camera& cameraData)
    {
        out_vis.main_camera_visible_object_indexes.clear();
        out_vis.camera_ubo_data = cameraData;
        out_vis.frustum.Build(glm::inverse(cameraData.viewproj));

        auto is_visible = [&](const RenderObject& obj)
        {
            math::Aabb transformed_aabb = obj.aabb.Transform(obj.model_matrix);
            if (out_vis.frustum.CheckSphere(transformed_aabb))
                return true;
            else
                return false;
        };

        for (size_t i = 0; i < render_objects.size(); i++)
        {
            if (is_visible(render_objects[i]))
                out_vis.main_camera_visible_object_indexes.push_back((uint32_t)i);
        }
		
    }

}