#ifndef _E_SYSTEMS_HPP__
#define _E_SYSTEMS_HPP__

#pragma once

#include "Core/EngineContext.hpp"
#include "ECS/Scene/Scene.hpp"

namespace Sowa {
enum SystemsFlags : uint32_t {
	SpriteRenderer2D = 1 << 0,
	Camera2DLogic = 1 << 8,

	// Components that needs to be updated every frame no matter what
	Update_Draw = SpriteRenderer2D,

	// Components that needs to be updated only when game is running
	Update_Logic = Camera2DLogic,

	// Drawing of Components that use mouse picking
	// Update_PickingDraw = ButtonPicking,
};
}

namespace Sowa::Systems {
// Updates all systems.
void ProcessAll(Sowa::Scene *pScene, SystemsFlags flags, EngineContext &ctx, bool pickable = false);

// Specific Systems
void System_Sprite2D(Sowa::Scene *pScene, bool pickable = false);
void System_Camera2DLogic(Sowa::Scene *pScene);

} // namespace Sowa::Systems

#endif