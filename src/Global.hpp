#pragma once


#include "entt/entt.hpp"
#include "Project/Project.hpp"

namespace Ease
{
class SceneTree;

namespace Global
{


extern Project* project;
extern SceneTree sceneTree;
extern entt::registry registry;

} // namespace Global   
} // namespace Ease
