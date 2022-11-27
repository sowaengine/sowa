#ifndef _E_SYSTEMS_HPP__
#define _E_SYSTEMS_HPP__

#pragma once

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

	// Components that needs to be updated every frame no matter what
	Update_Draw = SpriteRenderer2D | AnimatedSprite2D | TextRenderer2D | Physics2DDebugDraw | ButtonDraw,

	// Components that needs to be updated only when game is running
	Update_Logic = PhysicsBody2D | ButtonLogic | Camera2DLogic,

	// Drawing of Components that use mouse picking
	Update_PickingDraw = ButtonPicking,
};
}

namespace Sowa::Systems {
// Updates all systems.
void ProcessAll(Sowa::Scene *pScene, SystemsFlags flags, bool pickable = false);

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

} // namespace Sowa::Systems

#endif