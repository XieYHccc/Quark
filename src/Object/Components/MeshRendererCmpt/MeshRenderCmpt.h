#pragma once

#include <memory>

#include "../../../render/mesh_renderer.h"
#include "../component.h"
#include "../../../geometry/bounding_box.h"
class Object;
class MeshRendererCmpt : public Component {
public:
    MeshRendererCmpt(Object* object) : renderer_(), Component(object) {}

public: 
    void render();

    BoundingBox get_boundingbox() { return aabb_; }

    void set_material(std::shared_ptr<Material> material) { renderer_.set_material(material); }
    void set_shader(std::shared_ptr<Shader> shader) { renderer_.set_shader(shader); }
    
private:
    MeshRenderer renderer_;
    BoundingBox aabb_;
};