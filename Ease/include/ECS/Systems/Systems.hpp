#ifndef _E_SYSTEMS_HPP__
#define _E_SYSTEMS_HPP__

#pragma once

#include "ECS/Scene/Scene.hpp"

namespace Ease
{
    enum SystemsFlags : uint32_t
    {
        SpriteRenderer2D    = 1 << 0,
        AnimatedSprite2D    = 1 << 1,
        TextRenderer2D      = 1 << 2,
        PhysicsBody2D       = 1 << 3,
        Physics2DDebugDraw  = 1 << 4,
        ButtonDraw          = 1 << 5,
        ButtonPicking       = 1 << 6,
        ButtonLogic         = 1 << 7,
        Camera2DLogic       = 1 << 8,
        NativeBehaviourLogic     = 1 << 9,
        

        // Components that needs to be updated every frame no matter what
        Update_Draw         = SpriteRenderer2D | AnimatedSprite2D | TextRenderer2D | Physics2DDebugDraw | ButtonDraw,

        // Components that needs to be updated only when game is running
        Update_Logic  = PhysicsBody2D | ButtonLogic | Camera2DLogic | NativeBehaviourLogic,
        
        // Drawing of Components that use mouse picking
        Update_PickingDraw = ButtonPicking,
    };
}

namespace Ease::Systems
{
    // Updates all systems.
    void ProcessAll(Ease::Scene* pScene, SystemsFlags flags, bool pickable = false);


    // Specific Systems
    void System_SpriteRenderer2D(Ease::Scene* pScene, bool pickable = false);
    void System_AnimatedSprite2D(Ease::Scene* pScene, bool pickable = false);
    void System_TextRenderer2D(Ease::Scene* pScene, bool pickable = false);
    void System_PhysicsBody2D(Ease::Scene* pScene);
    void System_Physics2DDebugDraw(Ease::Scene* pScene, bool pickable = false);
    void System_ButtonDraw(Ease::Scene* pScene, bool pickable = false);
    void System_ButtonPicking(Ease::Scene* pScene);
    void System_ButtonLogic(Ease::Scene* pScene);
    void System_Camera2DLogic(Ease::Scene* pScene);
    void System_NativeBehaviour(Ease::Scene* pScene);

} // namespace Ease::Systems


#endif