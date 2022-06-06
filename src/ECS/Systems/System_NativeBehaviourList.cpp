#include "Systems.h"
#include "ECS/Scene/Scene.h"

#include "Resource/ResourceManager.h"
#include "Resource/NativeBehaviour/NativeBehaviour.h"
#include "ECS/Components/NativeBehaviourList/NativeBehaviourList.h"


namespace Ease::Systems
{
   void System_NativeBehaviourList(Ease::Scene* pScene)
   {
      static ResourceManager<Ease::NativeBehaviour>& loader_nativeBehaviour = ResourceManager<Ease::NativeBehaviour>::GetLoader();
      auto view = pScene->m_Registry.view<Component::NativeBehaviourList>();
      for(const auto& entityID : view)
      {
         Entity entity(entityID, &pScene->m_Registry);
         Component::NativeBehaviourList& nblist = entity.GetComponent<Component::NativeBehaviourList>();
         std::vector<Ease::BaseBehaviour*>& behaviourList = nblist.GetBehaviourList();

         
         for(Ease::BaseBehaviour* behaviour : behaviourList)
         {
            behaviour->self = entity;
            behaviour->Update();
         }
      }
   }
} // namespace Ease::Systems
