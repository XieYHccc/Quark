#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include "Quark/Ecs/Component.h"
namespace quark {

class TransformCmpt : public Component{
public:
    TransformCmpt();
    
    QK_COMPONENT_TYPE_DECL(TransformCmpt)

    glm::vec3 GetPosition() { return position_; }
    glm::quat GetQuat() { return quaternion_; }
    glm::vec3 GetScale()  { return scale_; }
    glm::mat4& GetTRSMatrix();
    glm::mat4 GetWorldMatrix();

    void SetQuat(const glm::quat& quat);
    void SetEuler(const glm::vec3& euler_angle);
    void SetPosition(const glm::vec3& position);
    void SetScale(const glm::vec3& scale);
    void SetTRSMatrix(const glm::mat4& trs);

private:
    void CalculateTRSMatrix();

    // Local sapce
    glm::quat quaternion_;
    glm::vec3 position_;
    glm::vec3 scale_;
    glm::mat4 TRSMatrix_;
    bool TRSMatrixIsDirty;

};

} // namespace quark