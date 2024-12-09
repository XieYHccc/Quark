#pragma once
#include "Quark/Render/RenderTypes.h"


namespace quark
{
	class RenderScene
	{
	public:
		// lights

		// render entities
		std::vector<RenderEntity> renderEntities;

		// axex, for editor
		std::optional<RenderEntity> axisRenderEntity;
	
	};
}