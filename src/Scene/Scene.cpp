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

#include "yaml-cpp/yaml.h"
#include <iostream>
#include <fstream>
#include <chrono>
#include <random>

#include "Debug.hpp"

Scene::Scene()
{
   auto ticks = std::chrono::high_resolution_clock::now().time_since_epoch().count();
   for(int i=0; i<ticks%256; i++)
      rand();
}
static uint32_t GenerateRandom()
{
   return rand();
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

Entity Scene::Create(const std::string& name, int targetID /*= -1*/, uint32_t targetUUID /*= 0*/) 
{
   entt::entity id =
      targetID == -1 ? m_Registry.create()
                     : m_Registry.create((entt::entity)targetID);
   Entity entity(id, &m_Registry);
   
   entity.addComponent<CommonComponent>(name);
   entity.getComponent<CommonComponent>().id = targetUUID == 0 ? GenerateRandom() : targetUUID;

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


