#pragma once
#include "Quark/Ecs/Component.h"
#include "Quark/Core/UUID.h"
#include "Quark/Asset/Asset.h"

#include <glm/glm.hpp>

namespace quark
{
	struct ArmatureCmpt : public Component
	{
		QK_COMPONENT_TYPE_DECL(ArmatureCmpt)

		std::vector<Entity*> bone_entities;
		Entity* root_bone_entity = nullptr;
		std::unordered_map<uint32_t, Entity*> bone_index_to_entity_map;
		std::vector<glm::mat4> joint_matrices;
		AssetID skeleton_asset_id;
		
	};

	struct AnimationCmpt : public Component
	{
		QK_COMPONENT_TYPE_DECL(AnimationCmpt)

		AssetID animation_asset_id;

		float current_time = 0.0f;
		float speed = 1.0f;
		bool playing = false;
		bool loop = false;
	};
}