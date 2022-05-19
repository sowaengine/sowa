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
        

        Entity Create(const std::string& name);

        entt::registry m_Registry;
    private:
    };
} // namespace Ease

#endif