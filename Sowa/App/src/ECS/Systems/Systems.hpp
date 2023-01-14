#ifndef _E_SYSTEMS_HPP__
#define _E_SYSTEMS_HPP__

#pragma once

#include "Core/EngineContext.hpp"
#include "ECS/Scene/Scene.hpp"

namespace Sowa {
enum SystemsFlags : uint32_t {
	SpriteRenderer2D = 1 << 0,
	AnimatedSprite2D = 1 << 1,
	TextRenderer2D = 1 << 2,
	PhysicsBody2D = 1 << 3,
	Physics2DDebugDraw = 1 << 4,
	ButtonDraw = 1 << 5,
	ButtonPicking = 1 << 6,
	ButtonLogic = 1 << 7,
	Camera2DLogic = 1 << 8,
	ScriptContainerLogic = 1 << 9,

	// Components that needs to be updated every frame no matter what
	Update_Draw = SpriteRenderer2D,

	// Components that needs to be updated only when game is running
	Update_Logic = Camera2DLogic | ScriptContainerLogic,

	// Drawing of Components that use mouse picking
	// Update_PickingDraw = ButtonPicking,
};
}

namespace Sowa::Systems {
// Updates all systems.
void ProcessAll(Sowa::Scene *pScene, SystemsFlags flags, EngineContext &ctx, bool pickable = false);

// Specific Systems
void System_Sprite2D(Sowa::Scene *pScene, bool pickable = false);
void System_AnimatedSprite2D(Sowa::Scene *pScene, bool pickable = false);
void System_Text2D(Sowa::Scene *pScene, bool pickable = false);
void System_PhysicsBody2D(Sowa::Scene *pScene);
void System_Physics2DDebugDraw(Sowa::Scene *pScene, bool pickable = false);
void System_ButtonDraw(Sowa::Scene *pScene, bool pickable = false);
void System_ButtonPicking(Sowa::Scene *pScene);
void System_ButtonLogic(Sowa::Scene *pScene);
void System_Camera2DLogic(Sowa::Scene *pScene);
void System_ScriptContainerLogic(Sowa::Scene *pScene, EngineContext &ctx);

} // namespace Sowa::Systems

#endif