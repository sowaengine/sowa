#include "ECS/Entity/Entity.hpp"

namespace Ease
{
   Entity::Entity()
   {

   }
   
   Entity::Entity(entt::entity entityID, entt::registry* pRegistry)
   {
      SetEntityID(entityID);
      SetRegistry(pRegistry);
   }

   Entity::~Entity()
   {

   }
} // namespace Ease