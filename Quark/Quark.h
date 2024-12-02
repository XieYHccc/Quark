//
// Note:	this file is to be included in client applications ONLY
//			NEVER include this file anywhere in the engine codebase
//

#pragma once

#include <Quark/Core/Application.h>
#include <Quark/Core/Logger.h>
#include <Quark/Core/Input.h>
#include <Quark/Core/TimeStep.h>
#include <Quark/Core/Window.h>
#include <Quark/Core/Timer.h>

#include <Quark/Core/Math/Aabb.h>
#include <Quark/Core/Math/Frustum.h>

// Quark Job system
#include <Quark/Core/JobSystem.h>

// Quark Event system
#include <Quark/Events/EventManager.h>
#include <Quark/Events/ApplicationEvent.h>
#include <Quark/Events/KeyEvent.h>
#include <Quark/Events/MouseEvent.h>

// Quark Asset system
#include <Quark/Asset/AssetManager.h>
#include <Quark/Project/Project.h>
#include <Quark/Project/ProjectSerializer.h>

// Quark Graphic API
#include <Quark/RHI/Device.h>
#include <Quark/RHI/TextureFormatLayout.h>

// Quark Renderer
#include <Quark/Renderer/Renderer.h>

// Quark Scene API
#include <Quark/Ecs/Component.h>
#include <Quark/Ecs/Entity.h>
#include <Quark/Scene/Scene.h>
#include <Quark/Scene/SceneSerializer.h>
#include <Quark/Scene/Components/CommonCmpts.h>
#include <Quark/Scene/Components/TransformCmpt.h>
#include <Quark/Scene/Components/CameraCmpt.h>
#include <Quark/Scene/Components/MeshCmpt.h>

