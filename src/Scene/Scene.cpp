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


/**
  * SCENE SERIALIZATION
  * 
  **/

namespace YAML
{
   template<>
   struct convert<glm::vec2>
   {
      static Node encode(const glm::vec2& vec)
      {
         Node node;
         node.push_back(vec.x);
         node.push_back(vec.y);
         return node;
      }
      static bool decode(const Node& node, glm::vec2& vec)
      {
         if(!node.IsSequence() || node.size() != 2)
            return false;
         
         vec.x = node[0].as<float>(0);
         vec.y = node[1].as<float>(0);
         return true;
      }
   };
}

YAML::Emitter& operator<<(YAML::Emitter& out, const glm::vec2& vec)
{
   out << YAML::Flow;
   out << YAML::BeginSeq << vec.x << vec.y << YAML::EndSeq;
   return out;
}



static void SerializeEntity(YAML::Emitter& out, Entity& entity)
{
   out << YAML::BeginMap;
      out << YAML::Key << "Name" << YAML::Value << entity.GetName();
      out << YAML::Key << "ID" << YAML::Value << entity.getComponent<CommonComponent>().id;

      out << YAML::Key << "Components";
      out << YAML::Value << YAML::BeginMap; // Components
         // Transform2D
         if(entity.hasComponent<Transform2DComponent>())
         {
            out << YAML::Key << "Transform2D";
            out << YAML::BeginMap; // Transform2D
               auto& comp = entity.getComponent<Transform2DComponent>();
               
               out << YAML::Key << "Position" << YAML::Value << comp.position;
               out << YAML::Key << "Rotation" << YAML::Value << comp.rotation;
               out << YAML::Key << "Scale" << YAML::Value << comp.scale;
               out << YAML::Key << "Z Index" << YAML::Value << comp.zIndex;

            out << YAML::EndMap; // Transform2D
         }
         // Sprite
         if(entity.hasComponent<SpriteComponent>())
         {
            out << YAML::Key << "Sprite";
            out << YAML::BeginMap; // Sprite
               auto& comp = entity.getComponent<SpriteComponent>();
               
               out << YAML::Key << "Texture" << YAML::Value << comp.textureRes->GetID(); // TextureID

            out << YAML::EndMap; // Sprite
         }
         // LuaScript
         if(entity.hasComponent<LuaScriptComponent>())
         {
            out << YAML::Key << "LuaScript";
            out << YAML::BeginMap; // LuaScript
               auto& comp = entity.getComponent<LuaScriptComponent>();
               
               out << YAML::Key << "Script" << YAML::Value << comp.script->GetID(); // TextureID

            out << YAML::EndMap; // LuaScript
         }
         // Camera
         if(entity.hasComponent<CameraComponent>())
         {
            out << YAML::Key << "Camera";
            out << YAML::BeginMap; // Camera
               auto& comp = entity.getComponent<CameraComponent>();
               
               out << YAML::Key << "Current" << YAML::Value << comp.current; // TextureID

            out << YAML::EndMap; // Camera
         }
      out << YAML::EndMap; // Components
   out << YAML::EndMap;
}
//static
bool Scene::SaveScene(const std::string& path, Scene& scene)
{
   YAML::Emitter out;
   out << YAML::Comment("Ease Scene File") << YAML::Newline;
   out << YAML::Comment("Don't modify if you don't know what are you doing.") << YAML::Newline;

   out << YAML::BeginMap;
      out << YAML::Key << "Scene" << YAML::Value << "New Scene";
      out << YAML::Key << "Resources" << YAML::Value << YAML::BeginSeq;
         std::vector<Resource*> resources = ResourceManager::get_singleton().GetResources();
         for(Resource* resource : resources)
         {
            if(resource == nullptr) continue;

            out << YAML::BeginMap;
               out << YAML::Key << "Type" << YAML::Value << resource->m_Name;
               out << YAML::Key << "ID" << YAML::Value << resource->m_ResourceID;
               for(ResourceProperty* property : resource->properties)
               {
                  out << YAML::Key << property->name;
                  if(property->type == PropertyType::STRING || property->type == PropertyType::SCRIPT || property->type == PropertyType::TEXTURE)
                     out << YAML::Value << property->dataStr;
               }
            out << YAML::EndMap;
         }
      out << YAML::EndSeq << YAML::Newline;

      out << YAML::Key << "Entities" << YAML::Value << YAML::BeginSeq;
         auto view = scene.GetRegistry().view<CommonComponent>();
         for(auto it = view.rbegin(); it != view.rend(); ++it)
         {
            Entity entity(*it, &scene.GetRegistry());
            
            SerializeEntity(out, entity);
         }
      out << YAML::EndSeq;
   out << YAML::EndMap;
   
   std::ofstream ofstream(path);
   ofstream << out.c_str();
   return true;
}


std::shared_ptr<TextureResource> GetResourceFromList(const std::vector<std::shared_ptr<TextureResource>>& list, uint32_t id)
{
   for(std::shared_ptr<TextureResource> resource : list)
   {
      if(resource->GetID() == id)
         return resource;
   }
   LOG_ERROR("Cannot find texture with id: %d", id);
   return nullptr;
}
std::shared_ptr<LuaScript> GetResourceFromList(const std::vector<std::shared_ptr<LuaScript>>& list, uint32_t id)
{
   for(std::shared_ptr<LuaScript> resource : list)
   {
      if(resource->GetID() == id)
         return resource;
   }
   LOG_ERROR("Cannot find texture with id: %d", id);
   return nullptr;
}

//static
bool Scene::LoadScene(const std::string& path, Scene& scene)
{
   auto& registry = scene.GetRegistry();
   registry.clear();

   std::ifstream ifstream(path);
   std::stringstream sstream;
   sstream << ifstream.rdbuf();

   YAML::Node sceneFile = YAML::Load(sstream.str());
   if(!sceneFile["Scene"])
      return false;
   scene.SetName(sceneFile["Scene"].as<std::string>("New Scene"));

   ResourceManager& resManager = ResourceManager::get_singleton();
   auto resources = sceneFile["Resources"];
   std::vector<std::shared_ptr<LuaScript>> luaScripts; // for keeping resource objects alive until function ends (if they are not used by any entity)
   std::vector<std::shared_ptr<TextureResource>> textures;
   if(resources)
   {
      for(auto resource : resources)
      {
         std::string type = resource["Type"].as<std::string>("None");
         uint32_t id = resource["ID"].as<uint32_t>(0);
         if(type == "Texture")
         {
            std::shared_ptr<TextureResource> tex = resManager.NewTextureResource(id);
            std::string path = resource["Texture"].as<std::string>("");
            tex->prop_filepath.dataStr = path;
            if(path != "")
               tex->PropertiesUpdated(); // loads the texture

            textures.push_back(tex);
         }
         else if(type == "LuaScript")
         {
            std::shared_ptr<LuaScript> script = resManager.NewLuaScript(id);
            std::string path = resource["Script"].as<std::string>("");
            script->prop_filepath.dataStr = path;
            if(path != "")
               script->PropertiesUpdated();

            luaScripts.push_back(script);
         }
      }
   }

   auto entities = sceneFile["Entities"];
   if(entities)
   {
      for(auto yaml_entity : entities)
      {
         std::string name = yaml_entity["Name"].as<std::string>("New Entity");
         uint32_t id = yaml_entity["ID"].as<uint32_t>(0);

         Entity entity = scene.Create(name, -1, id);

         auto components = yaml_entity["Components"];
         if(components)
         {
            auto transform2d = components["Transform2D"];
            if(transform2d)
            {
               auto& comp = entity.addComponent<Transform2DComponent>();

               comp.position = transform2d["Position"].as<glm::vec2>(glm::vec2(0.f, 0.f));
               comp.rotation = transform2d["Rotation"].as<float>(0.f);
               comp.scale = transform2d["Scale"].as<glm::vec2>(glm::vec2(1.f, 1.f));
               comp.zIndex = transform2d["Z Index"].as<int>(0);
            }

            auto sprite = components["Sprite"];
            if(sprite)
            {
               auto& comp = entity.addComponent<SpriteComponent>();

               uint32_t texID = sprite["Texture"].as<uint32_t>(0);
               if(texID == 0)
                  comp.textureRes = resManager.NewTextureResource();
               else
                  comp.textureRes = GetResourceFromList(textures, texID);
            }

            auto LuaScript = components["LuaScript"];
            if(LuaScript)
            {
               auto& comp = entity.addComponent<LuaScriptComponent>();

               uint32_t scriptID = LuaScript["Script"].as<uint32_t>(0);
               if(scriptID == 0)
                  comp.script = resManager.NewLuaScript();
               else
                  comp.script = GetResourceFromList(luaScripts, scriptID);
            }

            auto camera = components["Camera"];
            if(camera)
            {
               auto& comp = entity.addComponent<CameraComponent>();

               comp.current = camera["Current"].as<bool>(false);
            }
         }
      }
   }


   return true;
}