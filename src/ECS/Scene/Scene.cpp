#include "Scene.h"
#include "ECS/Components/Components.hpp"

namespace Ease
{
   Scene::Scene()
   {

   }

   Scene::~Scene()
   {

   }
   
   Entity Scene::Create(const std::string& name)
   {
      Entity entity;
      entity.SetEntityID(m_Registry.create());
      entity.SetRegistry(&m_Registry);

      auto& common = entity.AddComponent<Component::Common>();
      common.Name() = name;

      return entity;
   }
} // namespace Ease