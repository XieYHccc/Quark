#pragma once
#include <Quark/Scene/Scene.h>
#include "Editor/UI/Common.h"
namespace editor::ui {
class Inspector : public Window {

public:
    Inspector() : selectedNode_(nullptr) {}
    Inspector(scene::Node* node) : selectedNode_(node) {}

    virtual void Render() override;

    void SetNode(scene::Node* node) { selectedNode_ = node; }

private:
    void DrawComponents(scene::Entity* entity);

    scene::Node* selectedNode_;
    bool rename_ = false;
    char buf_[128] = "";

};
}