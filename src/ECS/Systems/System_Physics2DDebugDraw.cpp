#include "Systems.hpp"
#include "ECS/Scene/Scene.hpp"

#include "ECS/Components/Transform2D/Transform2D.hpp"
#include "ECS/Components/PhysicsBody2D/PhysicsBody2D.hpp"

#include "Core/Renderer.hpp"
#include "box2d/box2d.h"
#include "rlgl.h"
#include "Core/ProjectSettings.hpp"

namespace Ease::Systems
{
   void System_Physics2DDebugDraw(Ease::Scene* pScene, bool pickable /*= false*/)
    {
        if(!Ease::ProjectSettings::get_singleton().debug_draw)
            return;
        
        auto view = pScene->m_Registry.view<Component::Transform2D, Component::PhysicsBody2D>();
        for(const auto& entityID : view)
        {
            Entity entity(entityID, &pScene->m_Registry);
            auto& transformc = entity.GetComponent<Component::Transform2D>();
            auto& body = entity.GetComponent<Component::PhysicsBody2D>();
            
            for(Collider2D& collider : body.Colliders())
            {
                if(collider.shape == ColliderShape2D::BOX)
                {
                    if(!pickable)
                    {
                        Renderer::get_singleton().DrawQuadColor(
                            transformc.Position(),
                            { collider.width, collider.height },
                            transformc.Rotation(),
                            (Color){60, 20, 150, 100},
                            {collider.offset.x, -collider.offset.y}
                        );
                    }
                }
                else if(collider.shape == ColliderShape2D::CIRCLE)
                {
                    if(!pickable)
                    {
                        Renderer::get_singleton().DrawCircleColor(
                            transformc.Position(),
                            collider.radius,
                            transformc.Rotation(),
                            (Color){60, 20, 150, 100},
                            {collider.offset.x, -collider.offset.y}
                        );
                    }
                }
            }
        }
    }
} // namespace Ease::Systems