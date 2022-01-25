/**
 * @file Scene.cpp
 * @author Lexographics
 * @brief 
 * @version 0.1
 * @date 2022-01-26
 */
#include "Scene.hpp"  
#include "entt/entt.hpp"
#include "Entity.hpp"
#include "Components.hpp"

#include <iostream>

Scene::Scene()
{
}
	
Scene::~Scene()
{
}

//static
void Scene::CopyScene(Scene& src, Scene& dst)
{
   dst.ClearScene();
   
   auto view = src.GetRegistry().view<CommonComponent>();
   for(auto it = view.rbegin(); it != view.rend(); ++it)
   {
      Entity srcEntity(*it, &src.GetRegistry());
      auto& common = srcEntity.getComponent<CommonComponent>();
      
      Entity dstEntity = dst.Create(common.name, srcEntity.ID());

      // Copy components
      if(srcEntity.hasComponent<Transform2DComponent>())
      {
         auto& srcComp = srcEntity.getComponent<Transform2DComponent>();
         auto& dstComp = dstEntity.addComponent<Transform2DComponent>();
         dstComp = srcComp;
      }
      if(srcEntity.hasComponent<SpriteComponent>())
      {
         auto& srcComp = srcEntity.getComponent<SpriteComponent>();
         auto& dstComp = dstEntity.addComponent<SpriteComponent>();
         dstComp = srcComp;
      }
      if(srcEntity.hasComponent<CameraComponent>())
      {
         auto& srcComp = srcEntity.getComponent<CameraComponent>();
         auto& dstComp = dstEntity.addComponent<CameraComponent>();
         dstComp = srcComp;
      }
   }

}

void Scene::ClearScene()
{
   auto view = m_Registry.view<CommonComponent>();
   m_Registry.destroy(view.begin(), view.end());
}

Entity Scene::Create(const std::string& name, int targetID) 
{
   entt::entity id =
      targetID == -1 ? m_Registry.create()
                     : m_Registry.create((entt::entity)targetID);
   Entity entity(id, &m_Registry);
   
   entity.addComponent<CommonComponent>(name);

   return entity;
}