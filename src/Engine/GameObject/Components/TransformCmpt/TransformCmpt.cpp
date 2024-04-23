#include "GameObject/Components/TransformCmpt/TransformCmpt.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/transform.hpp>
#include <glm/gtx/quaternion.hpp>
glm::mat4 TransformCmpt::GetTRSMatrix()
{
    if (TRSMatrixIsDirty) {
        CalculateTRSMatrix();
    }
    return TRSMatrix_;
}

void TransformCmpt::CalculateTRSMatrix()
{
    glm::mat4 scale = glm::scale(scale_);
    glm::mat4 rotate = glm::toMat4(quaternion_);
    glm::mat4 translate = glm::translate(position_);
    TRSMatrix_ = translate * rotate * scale;
    TRSMatrixIsDirty = false;

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