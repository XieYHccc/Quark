#pragma once
#include "Quark/Asset/Asset.h"
#include "Quark/Core/Math/Aabb.h"
#include <glm/glm.hpp>

namespace quark 
{
    struct MeshSectionDesc 
    {
        uint32_t index_offset;
        uint32_t index_count;

        math::Aabb aabb;
        AssetID material_asset_id;;
    };

    struct StaticMeshRenderProxy 
    {
        uint32_t entity_id;
        AssetID mesh_asset_id;
        glm::mat4 transform;

        std::vector<MeshSectionDesc> mesh_sections;
    };

    struct CameraSwapData 
    {
        glm::mat4 view;
        glm::mat4 proj;
    };

    struct RenderSwapData 
    {
        std::vector<StaticMeshRenderProxy> dirty_static_mesh_render_proxies;
        std::vector<uint64_t> to_delete_entities;
        std::optional<CameraSwapData> camera_swap_data;
    };  

    class RenderSwapContext 
    {
    public:
        enum SwapDataType : uint8_t
        {
            LogicSwapDataType = 0,
            RenderSwapDataType,
            SwapDataTypeCount
        };

        RenderSwapData& GetLogicSwapData() { return m_swapData[m_logic_swap_data_index]; }
        RenderSwapData& GetRenderSwapData() { return m_swapData[m_render_swap_data_index]; }

        void SwapLogicRenderData();

    private:
        void Swap();
        bool IsReadyToSwap() const;

        uint8_t m_logic_swap_data_index = LogicSwapDataType;
        uint8_t m_render_swap_data_index = RenderSwapDataType;
        RenderSwapData m_swapData[SwapDataTypeCount];

    };


}