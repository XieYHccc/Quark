#pragma once
#include <Quark/Scene/Scene.h>
#include "Editor/UI/Common.h"

namespace quark {

class Inspector : public UIWindowBase {
public:
    Inspector() : selectedNode_(nullptr) {}
    Inspector(GameObject* node) : selectedNode_(node) {}

    virtual void Init() override;
    virtual void Render() override;

    void SetNode(GameObject* node) { selectedNode_ = node; }

private:
    GameObject* selectedNode_;
    bool rename_;
    char buf_[128];

};

}