#pragma once
#include <Quark/Scene/Scene.h>
#include "Editor/UI/Common.h"

namespace quark {

class Inspector : public UIWindowBase {
public:
    Inspector() : m_SelectedEntity(nullptr) {}
    Inspector(Entity* entity) : m_SelectedEntity(entity) {}

    virtual void Init() override;
    virtual void Render() override;

    void SetEntity(Entity* entity) { m_SelectedEntity = entity; }

private:
    Entity* m_SelectedEntity;
    bool rename_;
    char buf_[128];

};

}