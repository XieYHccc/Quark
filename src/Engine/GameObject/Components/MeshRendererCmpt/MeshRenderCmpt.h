#pragma once

#include <memory>

#include "GameObject/Components/Component.h"
#include "Graphics/Vulkan/VulkanTypes.h"

class MeshRendererCmpt : public Component {
public:
    GpuMeshVulkan* gpuMesh;
    bool isDirty;

public:
    MeshRendererCmpt(GameObject* owner) : 
        gpuMesh(nullptr),
        isDirty(true),
        Component(owner)
    {

    };

    COMPONENT_TYPE("MeshRenderer");

    const std::vector<RenderObject>& GetRenderObjects();


private:
    void GenerateRenderObjects();

    std::vector<RenderObject> renderObjects;
};