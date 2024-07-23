#pragma once
#include <Quark/Scene/Scene.h>
#include "Editor/UI/Common.h"

namespace editor::ui {
class SceneHeirarchy : public Window {

public:
    SceneHeirarchy();
    SceneHeirarchy(scene::Scene* scene);

    virtual void Render() override;

    void SetScene(scene::Scene* scene);

    scene::Node* GetSelectedNode() { return selectedNode_; }

private:
    void DrawNode(scene::Node* node);
    void DrawSceneSettings();

    scene::Scene* scene_;
    scene::Node* selectedNode_;

};
}