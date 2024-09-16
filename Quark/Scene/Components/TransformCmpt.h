#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include "Quark/Ecs/Component.h"
namespace quark {

class TransformCmpt : public Component{
public:
    QK_COMPONENT_TYPE_DECL(TransformCmpt)
    TransformCmpt();

    /// Local space
    glm::vec3 GetLocalPosition() { return m_localPosition; }
    glm::quat GetLocalRotate() { return m_localQuat; }
    glm::vec3 GetLocalScale() { return m_localScale; }
    glm::mat4 GetLocalMatrix();

    void SetLocalRotate(const glm::quat& quat);
    void SetLocalRotate(const glm::vec3& euler_angle);
    void SetLocalPosition(const glm::vec3& position);
    void SetLocalScale(const glm::vec3& scale);
    void SetLocalMatrix(const glm::mat4& trs);

    // World space
    glm::vec3 GetWorldPosition();
    glm::quat GetWorldRotate();
    glm::vec3 GetWorldScale();
    const glm::mat4& GetWorldMatrix();

    // Transformations
    void Translate(const glm::vec3& translation);
    void Rotate(const glm::quat& rotation);
    void Scale(const glm::vec3& scale);

private:
    void UpdateWorldMatrix();
    void UpdateWorldMatrix_Parent();

    void SetDirty(bool b);
    void SetParentDirty(bool b);

    bool IsDirty() const { return m_flags & Flags::DIRTY; }
    bool IsParentDirty() const { return m_flags & Flags::PARENT_DIRTY; }

    void PropagateDirtyFlagToChilds();

    enum Flags
    {
        NONE = 0,
        DIRTY = 1 << 0, // Local transform has changed
        PARENT_DIRTY = 1 << 1 // Need to update parent world matrix
    };
    uint32_t m_flags = Flags::DIRTY;

    glm::quat m_localQuat;
    glm::vec3 m_localPosition;
    glm::vec3 m_localScale;

    glm::mat4 m_worldMatrix;
    glm::mat4 m_parentWorldMatrix;

    friend class Scene;
};

} // namespace quark