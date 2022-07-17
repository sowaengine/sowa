#include "Systems.hpp"
#include "ECS/Scene/Scene.hpp"

#include "ECS/Components/Transform2D/Transform2D.hpp"
#include "ECS/Components/PhysicsBody2D/PhysicsBody2D.hpp"

#include "Core/Renderer.hpp"
#include "box2d/box2d.h"
#include "rlgl.h"
#include <iostream>

namespace Ease::Systems
{
   void System_PhysicsBody2D(Ease::Scene* pScene)
    {
        float timeStep = 1.f / 60.f;
        std::shared_ptr<b2World> world = pScene->PhysicsWorld2D();

        world->Step(timeStep, 6, 2);

        auto view = pScene->m_Registry.view<Component::Transform2D, Component::PhysicsBody2D>();
        for(const auto& entityID : view)
        {
            Entity entity(entityID, &pScene->m_Registry);
            auto& transformc = entity.GetComponent<Component::Transform2D>();
            auto& body = entity.GetComponent<Component::PhysicsBody2D>();
            
            if(body.HasBody())
            {
                const b2Vec2& pos = body.Position();
                transformc.Position().x = WORLD_TO_SCREEN(pos.x);
                transformc.Position().y = WORLD_TO_SCREEN(pos.y);
                transformc.Rotation() = -body.Angle() * RAD2DEG;
            }
        }
    }
} // namespace Ease::Systems