#include "Quark/qkpch.h"
#include "Quark/Core/Math/Util.h"
#include "Quark/Scene/Scene.h"
#include "Quark/Scene/Components/TransformCmpt.h"
#include "Quark/Scene/Components/RelationshipCmpt.h"

#include <glm/gtx/transform.hpp>
#include <glm/gtx/quaternion.hpp>

namespace quark {

TransformCmpt::TransformCmpt() :
    m_localQuat(1.f, 0.f, 0.f, 0.f),
    m_localPosition(0.f),
    m_localScale(1.f),
    m_worldMatrix(1.f),
    m_parentWorldMatrix(1.f)
{

}

glm::mat4 TransformCmpt::GetLocalMatrix()
{
    glm::mat4 scale = glm::scale(m_localScale);
    glm::mat4 rotate = glm::toMat4(m_localQuat);
    glm::mat4 translate = glm::translate(m_localPosition);
    return translate * rotate * scale;
}

void TransformCmpt::SetLocalRotate(const glm::quat &quat)
{
    SetDirty(true);
    PropagateDirtyFlagToChilds();

    m_localQuat = quat;
}

void TransformCmpt::SetLocalRotate(const glm::vec3& euler_angle)
{
    SetDirty(true);
    PropagateDirtyFlagToChilds();

    m_localQuat = glm::quat(euler_angle); 
}

void TransformCmpt::SetLocalPosition(const glm::vec3& position)
{
    SetDirty(true);
    PropagateDirtyFlagToChilds();

    m_localPosition = position;
}

void TransformCmpt::SetLocalScale(const glm::vec3& scale)
{
    SetDirty(true);
    PropagateDirtyFlagToChilds();

    m_localScale = scale;
}

void TransformCmpt::SetLocalMatrix(const glm::mat4 &trs)
{
    SetDirty(true);
    PropagateDirtyFlagToChilds();

    math::DecomposeTransform(trs, m_localPosition , m_localQuat, m_localScale);
}

glm::vec3 TransformCmpt::GetWorldPosition()
{
    if (IsParentDirty())
    {
        UpdateWorldMatrix_Parent();
		SetParentDirty(false);
        SetDirty(false);
    }

    if (IsDirty())
    {
        UpdateWorldMatrix();
        SetDirty(false);
    }

    return glm::vec3(m_worldMatrix[3]);
}

glm::quat TransformCmpt::GetWorldRotate()
{
    if (IsParentDirty())
    {
        UpdateWorldMatrix_Parent();
        SetParentDirty(false);
        SetDirty(false);
    }

    if (IsDirty())
    {
        UpdateWorldMatrix();
        SetDirty(false);
    }

    glm::vec3 translate;
    glm::vec3 scale;
    glm::quat rotate;
    math::DecomposeTransform(m_worldMatrix, translate, rotate, scale);

    return rotate;
}

glm::vec3 TransformCmpt::GetWorldScale()
{
    if (IsParentDirty())
    {
        UpdateWorldMatrix_Parent();
        SetParentDirty(false);
        SetDirty(false);
    }

    if (IsDirty())
    {
        UpdateWorldMatrix();
        SetDirty(false);
    }

    glm::vec3 translate;
    glm::vec3 scale;
    glm::quat rotate;
    math::DecomposeTransform(m_worldMatrix, translate, rotate, scale);

    return scale;
}

const glm::mat4& TransformCmpt::GetWorldMatrix()
{
    if (IsParentDirty())
    {
        UpdateWorldMatrix_Parent();
        SetParentDirty(false);
        SetDirty(false);
    }

    if (IsDirty())
    {
        UpdateWorldMatrix();
        SetDirty(false);
    }

    return m_worldMatrix;
}

void TransformCmpt::Translate(const glm::vec3& translation)
{
    SetDirty(true);
    PropagateDirtyFlagToChilds();

    m_localPosition.x += translation.x;
    m_localPosition.y += translation.y;
    m_localPosition.z += translation.z;
}

void TransformCmpt::Rotate(const glm::quat& rotation)
{
    SetDirty(true);
    PropagateDirtyFlagToChilds();

    glm::quat result = rotation * m_localQuat;
    m_localQuat = glm::normalize(result);
}

void TransformCmpt::Scale(const glm::vec3& scale)
{
    SetDirty(true);
    PropagateDirtyFlagToChilds();

    m_localScale.x *= scale.x;
    m_localScale.y *= scale.y;
    m_localScale.z *= scale.z;
}

void TransformCmpt::UpdateWorldMatrix()
{
    m_worldMatrix = GetLocalMatrix() * m_parentWorldMatrix;
}

void TransformCmpt::UpdateWorldMatrix_Parent()
{
    Entity* parent = GetEntity()->GetComponent<RelationshipCmpt>()->GetParentEntity();
    QK_CORE_VERIFY(parent);

    auto* parent_transform = parent->GetComponent<TransformCmpt>();
    UpdateWorldMatrix();
}

void TransformCmpt::SetDirty(bool b)
{
    if (b)
		m_flags |= Flags::DIRTY;
	else
		m_flags &= ~Flags::DIRTY;
}

void TransformCmpt::SetParentDirty(bool b)
{
	if (b)
		m_flags |= Flags::PARENT_DIRTY;
	else
		m_flags &= ~Flags::PARENT_DIRTY;
}

void TransformCmpt::PropagateDirtyFlagToChilds()
{
    auto* relationshipCmpt = GetEntity()->GetComponent<RelationshipCmpt>();
	for (auto* child : relationshipCmpt->GetChildEntities())
	{
		auto* transform = child->GetComponent<TransformCmpt>();
        if (!transform->IsParentDirty())
        {
            transform->SetParentDirty(true);
            transform->PropagateDirtyFlagToChilds();
        }
	}
}


}