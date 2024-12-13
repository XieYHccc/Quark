#include "Quark/qkpch.h"
#include "Quark/Render/RenderSwapContext.h"

namespace quark 
{
    void RenderSwapContext::SwapLogicRenderData()
    {
        if (IsReadyToSwap()) 
        {
            Swap();
        }
    }

    void RenderSwapContext::Swap()
    {
        std::swap(m_logic_swap_data_index, m_render_swap_data_index);
    }

    bool RenderSwapContext::IsReadyToSwap() const
    {
        return (m_swapData[m_render_swap_data_index].dirty_static_mesh_render_proxies.empty() &&
            m_swapData[m_render_swap_data_index].to_delete_entities.empty() &&
            !m_swapData[m_render_swap_data_index].camera_swap_data.has_value());
    }
}
