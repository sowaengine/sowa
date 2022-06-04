#ifndef SCENE_H
#define SCENE_H

#pragma once

#include "ECS/Entity/Entity.h"
#include "entt/entt.hpp"

namespace Ease
{
    class Scene
    {
    public:
        Scene();
        ~Scene();
        

        Entity Create(const std::string& name, uint32_t id = 0);

        entt::registry m_Registry;

        bool SaveToFile(const char* path);
        bool LoadFromFile(const char* path);
    private:
    };
} // namespace Ease

#endif