#pragma once
#include <glm/glm.hpp>
#include <Scene/Components/MoveControlCmpt.h>
namespace editor::component {

class EditorCameraControlCmpt : public scene::MoveControlCmpt {
public:
    QK_COMPONENT_TYPE_DECL(EditorCameraControlCmpt)
    
    using MoveControlCmpt::MoveControlCmpt;
    ~EditorCameraControlCmpt() = default;

    void Update(float deltaTime) override;

};
}