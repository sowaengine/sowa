#ifndef _E_SCENE_HPP__
#define _E_SCENE_HPP__

#pragma once

#include <filesystem>
#include <vector>
#include "entt/entt.hpp"
#include "box2d/box2d.h"
#include "ECS/Entity/Entity.hpp"

namespace Ease
{
    class Scene
    {
    public:
        Scene();
        ~Scene();
        

        Entity Create(const std::string& name, uint32_t id = 0);
        void Destroy(Ease::Entity& entity);

        entt::registry m_Registry;

        bool Save();
        bool SaveToFile(const char* file);
        bool LoadFromFile(const char* file);
        std::filesystem::path path;

        // Returns first entity that is in given group
        Ease::Entity GetEntityInGroup(const std::string& group);
        std::vector<Ease::Entity> GetEntitiesInGroup(const std::string& group);
        
        void StartScene();
        void UpdateScene();
        void StopScene();

        
        void CopyEntity(Ease::Entity entity);
        void AddCopiedEntity(Ease::Entity entity);
        void ClearCopiedEntities();
        Ease::Entity PasteCopiedEntity();
        std::vector<Ease::Entity> PasteCopiedEntities();
        uint32_t GetCopiedEntityCount();


        b2Vec2& Gravity() { return m_Gravity; }
        std::shared_ptr<b2World> PhysicsWorld2D() { return m_pPhysicsWorld2D; }
    private:
        /**
         * @brief Registry that holds copied entities
         */
        entt::registry m_CopyRegistry;

        b2Vec2 m_Gravity{0.f, -10.f};
        std::shared_ptr<b2World> m_pPhysicsWorld2D;
    };
} // namespace Ease

#endif