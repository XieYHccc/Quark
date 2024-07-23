#pragma once
#include <Quark/Scene/Scene.h>
#include "Editor/UI/Common.h"
namespace editor::ui {
class Inspector : public UIWindowBase {

public:
    Inspector() : selectedNode_(nullptr) {}
    Inspector(scene::Node* node) : selectedNode_(node) {}

    virtual void Init() override;
    virtual void Render() override;

    void SetNode(scene::Node* node) { selectedNode_ = node; }

private:
    void DrawComponents(scene::Entity* entity);

    scene::Node* selectedNode_;
    bool rename_;
    char buf_[128];

};
}