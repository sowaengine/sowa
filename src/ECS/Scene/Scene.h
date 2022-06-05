#ifndef SCENE_H
#define SCENE_H

#pragma once

#include "ECS/Entity/Entity.h"
#include "entt/entt.hpp"
#include <filesystem>

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
        
        void StartScene();
    private:
    };
} // namespace Ease

#endif