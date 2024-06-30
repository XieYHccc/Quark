#pragma once
#include "Graphic/Common.h"

namespace render {

class SceneRenderer {
public:

private:
    graphic::Device* graphic_device_;

    // Default textures and materials
    Ref<graphic::Image> whiteTexture;
    Ref<graphic::Image> BlackTexture;
    Ref<graphic::Image> errorCheckBoardTexture;
    
};
}