#ifndef _E_PHYSICSBODY2D_HPP__
#define _E_PHYSICSBODY2D_HPP__

#pragma once

#include <string>
#include "box2d/box2d.h"
#include "Ease.hpp"

namespace Ease
{
    enum class PhysicsBodyType
    {
        STATIC = 0,
        DYNAMIC = 1,
        KINEMATIC = 2
    };
    enum class ColliderShape2D
    {
        BOX = 0,
        CIRCLE,
    };
    struct Collider2D
    {
        ColliderShape2D shape = ColliderShape2D::BOX;
        b2Vec2 offset{0.f, 0.f};
        float rotation;
        int width = 128.f;
        int height = 128.f;
        int radius = 64.f;

        float density = 0.2f;
        float friction = 0.2f;
        float restitution = 0.f;
        float restitutionThreshold = 1.f;

        // internal
        b2PolygonShape polyShape;
        b2CircleShape circleShape;

        b2FixtureDef fixtureDef;
        b2Fixture* fixture;
    };

    class Scene;
}

namespace Ease::Component
{
    class PhysicsBody2D
    {
    public:
        PhysicsBody2D();
        ~PhysicsBody2D();


        PhysicsBodyType& BodyType() { return m_BodyType; }

        const b2Vec2& Position() { static b2Vec2 invalid; return m_b2Body ? m_b2Body->GetPosition() : invalid; }
        float Angle() { return m_b2Body->GetAngle(); }
        bool HasBody() { return m_b2Body != nullptr; }

        std::vector<Collider2D>& Colliders() { return m_Colliders; }
    private:
        PhysicsBodyType m_BodyType;

        std::vector<Collider2D> m_Colliders;


        // internal box2d
        friend class Ease::Scene;
        b2BodyDef m_b2BodyDef;
        b2Body* m_b2Body = nullptr;


        b2BodyType GetInternalBodyType();
    };
}

#endif