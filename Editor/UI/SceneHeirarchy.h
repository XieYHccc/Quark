#pragma once
#include <Quark/Scene/Scene.h>
#include "Editor/UI/Common.h"

namespace quark {
class SceneHeirarchy : public UIWindowBase {
public:
    SceneHeirarchy();
    SceneHeirarchy(Scene* scene);

    virtual void Init() override;
    virtual void Render() override;

    void SetScene(Scene* scene);

    GameObject* GetSelectedObject() { return selectedObject_; }

private:
    void DrawNode(GameObject* object);
    void DrawSceneSettings();

    Scene* scene_;
    GameObject* selectedObject_;

};

}