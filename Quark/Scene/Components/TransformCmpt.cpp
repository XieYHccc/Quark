#include "Quark/qkpch.h"

#include <glm/gtx/transform.hpp>
#include <glm/gtx/quaternion.hpp>

#include "Quark/Scene/Scene.h"
#include "Quark/Scene/Components/TransformCmpt.h"
#include "Quark/Scene/Components/RelationshipCmpt.h"

namespace quark {

TransformCmpt::TransformCmpt() :
    quaternion_(1.f, 0.f, 0.f, 0.f),
    position_(0.f),
    scale_(1.f),
    TRSMatrix_(1.f),
    TRSMatrixIsDirty(true)
{

}

glm::mat4& TransformCmpt::GetTRSMatrix()
{
    if (TRSMatrixIsDirty) {
        CalculateTRSMatrix();
        TRSMatrixIsDirty = false;
    }
    return TRSMatrix_;
}

void TransformCmpt::CalculateTRSMatrix()
{
    glm::mat4 scale = glm::scale(scale_);
    glm::mat4 rotate = glm::toMat4(quaternion_);
    glm::mat4 translate = glm::translate(position_);
    TRSMatrix_ = translate * rotate * scale;

}

void TransformCmpt::SetQuat(const glm::quat &quat)
{
    quaternion_ = quat;
    TRSMatrixIsDirty = true;
}

void TransformCmpt::SetEuler(const glm::vec3& euler_angle)
{
    quaternion_ = glm::quat(euler_angle); 
    TRSMatrixIsDirty = true;
}

void TransformCmpt::SetPosition(const glm::vec3& position)
{
    position_ = position;
    TRSMatrixIsDirty = true;
}

void TransformCmpt::SetScale(const glm::vec3& scale)
{
    scale_ = scale;
    TRSMatrixIsDirty = true;
}

void TransformCmpt::SetTRSMatrix(const glm::mat4 &trs)
{
    TRSMatrix_ = trs;
    TRSMatrixIsDirty = false;
}

glm::mat4 TransformCmpt::GetWorldMatrix()
{
    Entity* parent = GetEntity()->GetComponent<RelationshipCmpt>()->GetParentEntity();

    if (parent == nullptr) {
        return GetTRSMatrix();
    }
    else {
        auto* parent_transform = parent->GetComponent<TransformCmpt>();
        return parent_transform->GetWorldMatrix() * GetTRSMatrix();
    }
}

}