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
#include "Application/Application.hpp"

#include <iostream>

Scene::Scene()
{
}
	
Scene::~Scene()
{
}

template<typename T>
void CopyComponent(Entity& srcEntity, Entity& dstEntity)
{
   if(srcEntity.hasComponent<T>())
   {
      T& srcComp = srcEntity.getComponent<T>();
      T& dstComp = dstEntity.addComponent<T>();
      dstComp = srcComp;
   }
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
      CopyComponent<Transform2DComponent>(srcEntity, dstEntity);
      CopyComponent<SpriteComponent>(srcEntity, dstEntity);
      CopyComponent<CameraComponent>(srcEntity, dstEntity);
      CopyComponent<LuaScriptComponent>(srcEntity, dstEntity);

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


// static
Entity Scene::GetEntityByName(const std::string& name)
{
   entt::registry& registry = Application::get_singleton().GetCurrentScene().GetRegistry();
   auto view = registry.view<CommonComponent>();
   for(auto it = view.rbegin(); it != view.rend(); ++it)
   {
      Entity entity(*it, &registry);
      auto& common = entity.getComponent<CommonComponent>();
      
      if(common.name == name)
         return entity;
   }

   return Entity(entt::entity(0), nullptr);
}