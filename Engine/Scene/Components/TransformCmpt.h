#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include "Scene/Component.h"

namespace scene {

class TransformCmpt : public Component{
public:
    TransformCmpt(Entity* object) :
        quaternion_(1.f, 0.f, 0.f, 0.f),
        position_(0.f),
        scale_(1.f),
        TRSMatrix_(1.f),
        TRSMatrixIsDirty(true),
        Component(object)
    {

    }
    
    QK_COMPONENT_TYPE_DECL(TransformCmpt)

    glm::vec3 GetPosition() const { return position_; }
    glm::quat GetQuat() const { return quaternion_; }
    glm::vec3 GetScale() const { return scale_; }
    glm::mat4 GetTRSMatrix();

    void SetQuat(const glm::quat& quat);
    void SetEuler(const glm::vec3& euler_angle);
    void SetPosition(const glm::vec3& position);
    void SetScale(const glm::vec3& scale);
    void SetTRSMatrix(const glm::mat4& trs);

private:
    void CalculateTRSMatrix();

    glm::quat quaternion_;
    glm::vec3 position_;
    glm::vec3 scale_;
    glm::mat4 TRSMatrix_;
    bool TRSMatrixIsDirty;
};

}