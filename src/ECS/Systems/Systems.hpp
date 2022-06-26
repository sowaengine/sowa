#ifndef _E_SYSTEMS_HPP__
#define _E_SYSTEMS_HPP__

#pragma once

#include "ECS/Scene/Scene.hpp"

namespace Ease
{
    enum SystemsFlags
    {
        SpriteRenderer2D    = 1 << 0,
        AnimatedSprite2D    = 1 << 1,
        TextRenderer2D      = 1 << 2,
        PhysicsBody2D       = 1 << 3,
        Physics2DDebugDraw  = 1 << 4,
        
        // Components that needs to be updated every frame no matter what
        Update_Always       = SpriteRenderer2D | AnimatedSprite2D | TextRenderer2D | Physics2DDebugDraw,

        // Components that needs to be updated always or only if game is running
        Update_WhenRunning  = Update_Always | PhysicsBody2D,
    };
}

namespace Ease::Systems
{
    // Updates all systems.
    void ProcessAll(Ease::Scene* pScene, SystemsFlags flags = SystemsFlags::Update_Always);


    // Specific Systems
    void System_SpriteRenderer2D(Ease::Scene* pScene);
    void System_AnimatedSprite2D(Ease::Scene* pScene);
    void System_TextRenderer2D(Ease::Scene* pScene);
    void System_PhysicsBody2D(Ease::Scene* pScene);
    void System_Physics2DDebugDraw(Ease::Scene* pScene);

} // namespace Ease::Systems


#endif