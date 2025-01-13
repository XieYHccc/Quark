#include "Quark/qkpch.h"
#include "Quark/Scene/Scene.h"
#include "Quark/Scene/Components/CommonCmpts.h"
#include "Quark/Scene/Components/TransformCmpt.h"
#include "Quark/Scene/Components/MeshCmpt.h"
#include "Quark/Scene/Components/MeshRendererCmpt.h"
#include "Quark/Scene/Components/RelationshipCmpt.h"
#include "Quark/Scene/Components/CameraCmpt.h"
#include "Quark/Scene/Components/ArmatureComponent.h"
#include "Quark/Render/RenderSystem.h"
#include "Quark/Asset/AssetManager.h"
#include "Quark/Animation/SkeletonAsset.h"
#include "Quark/Animation/AnimationAsset.h"

namespace quark {

void Scene::BuildBoneEntities(Entity* bone_entity, uint32_t bone_index, ArmatureCmpt* armature_cmpt)
{
    auto skeleton_asset = AssetManager::Get().GetAsset<SkeletonAsset>(armature_cmpt->skeleton_asset_id);
    armature_cmpt->bone_index_to_entity_map[bone_index] = bone_entity;

    auto* transformCmpt = bone_entity->GetComponent<TransformCmpt>();
    transformCmpt->SetLocalPosition(skeleton_asset->bone_translations[bone_index]);
    transformCmpt->SetLocalRotate(skeleton_asset->bone_rotations[bone_index]);
    transformCmpt->SetLocalScale(skeleton_asset->bone_scales[bone_index]);

    // build child bone entities
    std::vector<uint32_t> child_bone_indices = skeleton_asset->GetChildBoneIndices(bone_index);
    for (auto child_index : child_bone_indices)
    {
        Entity* child_entity = CreateEntity(skeleton_asset->bone_names[child_index], bone_entity);
        BuildBoneEntities(child_entity, child_index, armature_cmpt);
    }
}

Scene::Scene(const std::string& name)
    : sceneName(name), m_main_camera_entity(nullptr)
{
}

Scene::~Scene()
{   

}


void Scene::DeleteEntity(Entity* entity)
{
    // Remove from parent
    auto* relationshipCmpt = entity->GetComponent<RelationshipCmpt>();
    if (relationshipCmpt->GetParentEntity())
    {
        auto* parentRelationshipCmpt = relationshipCmpt->GetParentEntity()->GetComponent<RelationshipCmpt>();
        parentRelationshipCmpt->RemoveChildEntity(entity);
    }

    // Iteratively delete children
    std::vector<Entity*> children = relationshipCmpt->GetChildEntities();
    for (auto* c: children)
        DeleteEntity(c);

    // Delete entity
    m_entity_registry.DeleteEntity(entity);
}

void Scene::AttachChild(Entity* child, Entity* parent)
{
    auto* childRelationshipCmpt = child->GetComponent<RelationshipCmpt>();
	auto* parentRelationshipCmpt = parent->GetComponent<RelationshipCmpt>();

	if (childRelationshipCmpt->GetParentEntity())
	{
		auto* oldParentRelationshipCmpt = childRelationshipCmpt->GetParentEntity()->GetComponent<RelationshipCmpt>();
		oldParentRelationshipCmpt->RemoveChildEntity(child);
	}

	parentRelationshipCmpt->AddChildEntity(child);
}

void Scene::DetachChild(Entity* child)
{
    auto* relationshipCmpt = child->GetComponent<RelationshipCmpt>();
    if (relationshipCmpt->GetParentEntity())
	{
		auto* parentRelationshipCmpt = relationshipCmpt->GetParentEntity()->GetComponent<RelationshipCmpt>();
		parentRelationshipCmpt->RemoveChildEntity(child);
	}
}

Entity* Scene::CreateEntity(const std::string& name, Entity* parent)
{
    return CreateEntityWithID({}, name, parent);
}

Entity* Scene::CreateEntityWithID(UUID id, const std::string& name, Entity* parent)
{
    // Create entity
    Entity* newEntity = m_entity_registry.CreateEntity();

    auto* idCmpt = newEntity->AddComponent<IdCmpt>();
    idCmpt->id = id;

    auto* relationshipCmpt = newEntity->AddComponent<RelationshipCmpt>();

    newEntity->AddComponent<TransformCmpt>();
    if (!name.empty())
    {
        newEntity->AddComponent<NameCmpt>(name);
    }

    if (parent != nullptr)
    {
        auto* parentRelationshipCmpt = parent->GetComponent<RelationshipCmpt>();
        parentRelationshipCmpt->AddChildEntity(newEntity);
    }

    QK_CORE_ASSERT(m_id_to_entity_map.find(id) == m_id_to_entity_map.end())
    m_id_to_entity_map[id] = newEntity;

    return newEntity;
}

Entity* Scene::GetEntityWithID(UUID id)
{
    auto find = m_id_to_entity_map.find(id);
    if (find != m_id_to_entity_map.end())
        return find->second;
    else
        return nullptr;
}

Entity* Scene::GetParentEntity(Entity* entity)
{
    auto* relationshipCmpt = entity->GetComponent<RelationshipCmpt>();
    return relationshipCmpt->GetParentEntity();
}

std::vector<Entity*> Scene::GetChildEntities(Entity* parent, bool recursive)
{
    std::vector<Entity*> result;

    auto* relationshipCmpt = parent->GetComponent<RelationshipCmpt>();
    std::vector<Entity*>& children = relationshipCmpt->GetChildEntities();

    if (!recursive)
    {
        result = children;
    }
    else if (children.empty())
    {
        for (auto* child : children)
		{
			result.push_back(child);
			auto childChildren = GetChildEntities(child, true);
            if (!childChildren.empty())
			    result.insert(result.end(), childChildren.begin(), childChildren.end());
		}
	}
    
    return result;

}

Entity* Scene::GetMainCameraEntity()
{
    if (m_main_camera_entity)
    {
        return m_main_camera_entity;
    }
    else 
    {
        return nullptr;
    }
}

void Scene::AddArmatureComponent(Entity* entity, Ref<SkeletonAsset> skeleton_asset)
{
	QK_CORE_VERIFY(!entity->HasComponent<ArmatureCmpt>());

    auto* armature_cmpt = entity->AddComponent<ArmatureCmpt>();
    armature_cmpt->skeleton_asset_id = skeleton_asset->GetAssetID();

    // build skeleton hierarchy
    Entity* root_bone_entity = CreateEntity(skeleton_asset->bone_names[skeleton_asset->root_bone_index], entity);
    BuildBoneEntities(root_bone_entity, skeleton_asset->root_bone_index, armature_cmpt);

    armature_cmpt->root_bone_entity = root_bone_entity;
    armature_cmpt->bone_entities.push_back(root_bone_entity);
    std::vector<Entity*> child_bone_entities = GetChildEntities(root_bone_entity, true);
    armature_cmpt->bone_entities.insert(armature_cmpt->bone_entities.end(), child_bone_entities.begin(), child_bone_entities.end());


}

void Scene::OnUpdate(TimeStep delta_time)
{
    RunAnimationUpdateSystem(delta_time);
    // RunTransformUpdateSystem();
    RunJointsUpdateSystem();
}

//void Scene::RunTransformUpdateSystem()
//{
//    auto& groupVector = GetComponents<TransformCmpt>();
//
//    for (auto& group : groupVector)
//    {
//        TransformCmpt* t = GetComponent<TransformCmpt>(group);
//        bool dirty = false;
//        if (t->IsParentDirty())
//        {
//            t->UpdateWorldMatrix_Parent();
//            t->SetParentDirty(false);
//            t->SetDirty(false);
//            dirty = true;
//        }
//        else if (t->IsDirty())
//        {
//            t->UpdateWorldMatrix();
//            t->SetDirty(false);
//            dirty = true;
//        }
//        
//        // mark render state dirty
//        if (dirty) 
//        {
//            auto* renderCmpt = t->GetEntity()->GetComponent<MeshRendererCmpt>();
//            if (renderCmpt)
//                renderCmpt->SetDirty(true);
//        }
//    }
//}

void Scene::RunAnimationUpdateSystem(TimeStep delta_time)
{
    auto& groupVector = GetComponents<AnimationCmpt, ArmatureCmpt, TransformCmpt>();

    for (auto& group : groupVector)
    {
        auto* animation_cmpt = GetComponent<AnimationCmpt>(group);
        auto* armature_cmpt = GetComponent<ArmatureCmpt>(group);
        auto* skin_entity_transform_cmpt = GetComponent<TransformCmpt>(group);

        auto skeleton_asset = AssetManager::Get().GetAsset<SkeletonAsset>(armature_cmpt->skeleton_asset_id);
        auto animation_asset = AssetManager::Get().GetAsset<AnimationAsset>(animation_cmpt->animation_asset_id);

        animation_cmpt->current_time += delta_time.GetSeconds();
        if (animation_cmpt->current_time > animation_asset->end)
		{
            animation_cmpt->current_time -= animation_asset->end;
		}

        for (auto& channel : animation_asset->channels)
        {
            AnimationSampler& sampler = animation_asset->samplers[channel.samplerIndex];
            Entity* bone_entity = armature_cmpt->bone_index_to_entity_map[channel.boneIndex];
            TransformCmpt* bone_transform_cmpt = bone_entity->GetComponent<TransformCmpt>();

            for (size_t i = 0; i < sampler.inputs.size() - 1; i++)
            {
                QK_CORE_ASSERT(sampler.interpolation == "LINEAR");

                // get the input keyframe values for the current time stamp
                if (animation_cmpt->current_time >= sampler.inputs[i] && animation_cmpt->current_time <= sampler.inputs[i + 1])
                {
                    float a = (animation_cmpt->current_time - sampler.inputs[i]) / (sampler.inputs[i + 1] - sampler.inputs[i]);
                    if (channel.path == "translation")
                    {
                        bone_transform_cmpt->SetLocalPosition(glm::mix(sampler.outputsVec4[i], sampler.outputsVec4[i + 1], a));
                    }

                    if (channel.path == "rotation")
                    {
                        glm::quat q1;
                        q1.x = sampler.outputsVec4[i].x;
                        q1.y = sampler.outputsVec4[i].y;
                        q1.z = sampler.outputsVec4[i].z;
                        q1.w = sampler.outputsVec4[i].w;

                        glm::quat q2;
                        q2.x = sampler.outputsVec4[i + 1].x;
                        q2.y = sampler.outputsVec4[i + 1].y;
                        q2.z = sampler.outputsVec4[i + 1].z;
                        q2.w = sampler.outputsVec4[i + 1].w;

                        bone_transform_cmpt->SetLocalRotate(glm::normalize(glm::slerp(q1, q2, a)));
                    }

                    if (channel.path == "scale")
                    {
                        bone_transform_cmpt->SetLocalScale(glm::mix(glm::vec3(sampler.outputsVec4[i]), glm::vec3(sampler.outputsVec4[i + 1]), a));
					}
                }

            }
        }
    }


}

void Scene::RunJointsUpdateSystem()
{
    auto& groupVector = GetComponents<ArmatureCmpt, TransformCmpt>();

    for (auto& group : groupVector)
    {
		auto* armature_cmpt = GetComponent<ArmatureCmpt>(group);
		auto* skin_entity_transform_cmpt = GetComponent<TransformCmpt>(group);

		auto skeleton_asset = AssetManager::Get().GetAsset<SkeletonAsset>(armature_cmpt->skeleton_asset_id);

		// calculate joint matrices
        armature_cmpt->joint_matrices.resize(skeleton_asset->bone_names.size());
        for (size_t i = 0; i < skeleton_asset->bone_names.size(); i++)
        {
            glm::mat4 inverse_world_transform = glm::inverse(skin_entity_transform_cmpt->GetWorldMatrix());
            Entity* bone_entity = armature_cmpt->bone_index_to_entity_map[i];
            TransformCmpt* transform_cmpt = bone_entity->GetComponent<TransformCmpt>();
            glm::mat4 joint_matrix = transform_cmpt->GetWorldMatrix() * skeleton_asset->inverse_bind_matrices[i];
            armature_cmpt->joint_matrices[i] = inverse_world_transform * joint_matrix;
            //QK_CORE_LOGT_TAG("ANIMATION", "{}: {}", i, armature_cmpt->joint_matrices[i][0][3]);
        }
    }
}

void Scene::FillMeshSwapData()
{
    auto& swapData = RenderSystem::Get().GetSwapContext().GetLogicSwapData();

    // static mesh render proxies
    const auto& cmpts = GetComponents<IdCmpt, MeshCmpt, MeshRendererCmpt, TransformCmpt>();
    for (const auto [id_cmpt, mesh_cmpt, mesh_renderer_cmpt, transform_cmpt] : cmpts)
    {
        if (!mesh_renderer_cmpt->IsRenderStateDirty())
            continue;
        
        mesh_renderer_cmpt->SetDirty(false);

        auto* mesh = mesh_cmpt->uniqueMesh ? mesh_cmpt->uniqueMesh.get() : mesh_cmpt->sharedMesh.get();
        if (!mesh) 
            continue;
        
        StaticMeshRenderProxy newRenderProxy;
        newRenderProxy.entity_id = id_cmpt->id;
        newRenderProxy.mesh_asset_id = mesh->GetAssetID();
        newRenderProxy.transform = transform_cmpt->GetWorldMatrix();

        for (uint32_t i = 0; i < mesh->subMeshes.size(); ++i) {
            const auto& submesh = mesh->subMeshes[i];
            MeshSectionDesc newSectionDesc;
            
            newSectionDesc.aabb = submesh.aabb;
            newSectionDesc.index_count = submesh.count;
            newSectionDesc.index_offset = submesh.startIndex;
            newSectionDesc.material_asset_id = mesh_renderer_cmpt->GetMaterialID(i);
            newRenderProxy.mesh_sections.push_back(newSectionDesc);
        }

        swapData.dirty_static_mesh_render_proxies.push_back(newRenderProxy);
    }

    // skin mesh render proxies
    const auto& skinCmpts = GetComponents<NameCmpt, IdCmpt, MeshCmpt, MeshRendererCmpt, TransformCmpt, ArmatureCmpt>();
    for (auto group : skinCmpts)
    {
        auto* nameCmpt = GetComponent<NameCmpt>(group);
        auto* idCmpt = GetComponent<IdCmpt>(group);
		auto* meshCmpt = GetComponent<MeshCmpt>(group);
		auto* meshRendererCmpt = GetComponent<MeshRendererCmpt>(group);
		auto* transformCmpt = GetComponent<TransformCmpt>(group);
		auto* armatureCmpt = GetComponent<ArmatureCmpt>(group);

		auto* mesh = meshCmpt->uniqueMesh ? meshCmpt->uniqueMesh.get() : meshCmpt->sharedMesh.get();
		if (!mesh)
			continue;

		StaticMeshRenderProxy newRenderProxy;
		newRenderProxy.entity_id = idCmpt->id;
		newRenderProxy.mesh_asset_id = mesh->GetAssetID();
		newRenderProxy.transform = transformCmpt->GetWorldMatrix();
        newRenderProxy.joint_matrices = armatureCmpt->joint_matrices;
		for (uint32_t i = 0; i < mesh->subMeshes.size(); ++i) {
			const auto& submesh = mesh->subMeshes[i];
			MeshSectionDesc newSectionDesc;

			newSectionDesc.aabb = submesh.aabb;
			newSectionDesc.index_count = submesh.count;
			newSectionDesc.index_offset = submesh.startIndex;
			newSectionDesc.material_asset_id = meshRendererCmpt->GetMaterialID(i);
			newRenderProxy.mesh_sections.push_back(newSectionDesc);
		}

		swapData.dirty_static_mesh_render_proxies.push_back(newRenderProxy);
	
    }
}

void Scene::FillCameraSwapData()
{
    auto& swapData = RenderSystem::Get().GetSwapContext().GetLogicSwapData();

    auto* mainCameraEntity = GetMainCameraEntity();
    QK_CORE_VERIFY(mainCameraEntity)
    auto* cameraCmpt = mainCameraEntity->GetComponent<CameraCmpt>();

    CameraSwapData cameraSwapData;
    cameraSwapData.view = cameraCmpt->GetViewMatrix();
    cameraSwapData.proj = cameraCmpt->GetProjectionMatrix();
    swapData.camera_swap_data = cameraSwapData;
}
}