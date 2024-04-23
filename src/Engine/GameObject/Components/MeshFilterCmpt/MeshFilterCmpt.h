#pragma once

#include <string>

#include "GameObject/Components/Component.h"

#include "Graphics/Vulkan/VulkanTypes.h"

class MeshFilterCmpt : public Component {
public:
    using Component::Component;
    COMPONENT_TYPE("MeshFilterCmpt");
    
};