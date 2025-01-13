#pragma once
#include <unordered_map>	
#include <string>
#include "Quark/Asset/Asset.h"
namespace quark {

inline static std::unordered_map<std::string, AssetType> s_AssetExtensionMap =
{
	// Hazel types
	{ ".qkscene", AssetType::SCENE },
	{ ".qkmesh", AssetType::MESH },
	{ ".qkmaterial", AssetType::MATERIAL },
	{ ".qkimage", AssetType::IMAGE},
	{ ".cs", AssetType::SCRIPT },

	// mesh/animation source
	{ ".fbx", AssetType::MESH },
	{ ".gltf", AssetType::MESH },
	{ ".glb", AssetType::MESH },
	{ ".obj", AssetType::MESH },
	{ ".dae", AssetType::MESH },

	// Textures
	{ ".png", AssetType::IMAGE },
	{ ".jpg", AssetType::IMAGE },
	{ ".jpeg", AssetType::IMAGE },
	{ ".hdr", AssetType::IMAGE },
	{ ".ktx2", AssetType::IMAGE},

	// Audio
	{ ".wav", AssetType::AUDIO },
	{ ".ogg", AssetType::AUDIO },

	// Fonts
	{ ".ttf", AssetType::FONT },
	{ ".ttc", AssetType::FONT },
	{ ".otf", AssetType::FONT },
};
}