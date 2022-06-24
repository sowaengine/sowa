#include "Scene.h"
#include "ECS/Components/Components.hpp"
#include <iostream>
#include <fstream>
#include "yaml-cpp/yaml.h"
#include "Utils/YAML.h"
#include "Core/Application.h"
#include "Resource/ResourceManager.h"
#include <filesystem>
#include "Core/ProjectSettings.h"

namespace Ease
{
   void Scene::StartScene()
   {
      auto view = m_Registry.view<Component::NativeBehaviourClass>();
      for(auto& e : view)
      {
         Entity entity(e, &m_Registry);
         
         Component::NativeBehaviourClass& nbehaviour = entity.GetComponent<Component::NativeBehaviourClass>();
         nbehaviour.Factory() = Application::get_singleton().GetFactory(nbehaviour.ClassName());

         nbehaviour.Behaviour() = nbehaviour.Factory()->Create();
         nbehaviour.Behaviour()->self = entity;
         nbehaviour.Behaviour()->Start();
      }
   }
   void Scene::UpdateScene()
   {
      auto view = m_Registry.view<Component::NativeBehaviourClass>();
      for(auto& e : view)
      {
         Entity entity(e, &m_Registry);
         
         Component::NativeBehaviourClass& nbehaviour = entity.GetComponent<Component::NativeBehaviourClass>();

         nbehaviour.Behaviour()->Update();
      }
   }
   void Scene::StopScene()
   {
      auto view = m_Registry.view<Component::NativeBehaviourClass>();
      for(auto& e : view)
      {
         Entity entity(e, &m_Registry);
         
         Component::NativeBehaviourClass& nbehaviour = entity.GetComponent<Component::NativeBehaviourClass>();

         nbehaviour.Behaviour()->self = Entity(entt::null, nullptr);
         nbehaviour.Factory()->Destroy(nbehaviour.Behaviour());
         // nbehaviour.Behaviour()->Start();
      }
   }
   

   void Scene::CopyEntity(Ease::Entity entity)
   {
      ClearCopiedEntities();
      AddCopiedEntity(entity);
   }
   
   template<typename T>
   void CopyComponent(Ease::Entity& srcEntity, Ease::Entity& dstEntity)
   {
      if(srcEntity.HasComponent<T>())
      {
         auto& srcComponent = srcEntity.GetComponent<T>();
         auto& dstComponent = dstEntity.AddComponent<T>();

         dstComponent = srcComponent;
      }
   }

   void Scene::AddCopiedEntity(Ease::Entity entity)
   {
      entt::entity copyID = m_CopyRegistry.create((entt::entity)entity.ID());
      Ease::Entity copyEntity(copyID, &m_CopyRegistry);

      // Copy components
      auto& common = copyEntity.AddComponent<Component::Common>();
      common.Name() = entity.GetComponent<Component::Common>().Name();

      CopyComponent<Component::Transform2D>(entity, copyEntity);
      CopyComponent<Component::TextRenderer2D>(entity, copyEntity);
      CopyComponent<Component::SpriteRenderer2D>(entity, copyEntity);
      CopyComponent<Component::Group>(entity, copyEntity);
   }
   
   void Scene::ClearCopiedEntities()
   {
      m_CopyRegistry.clear();
   }

   /**
    * @brief Create an Entity From {srcRegistry} on {scene}
    */
   Ease::Entity CreateEntityFromRegistry(entt::entity id, entt::registry& srcRegistry, Ease::Scene& scene)
   {
      Ease::Entity copyEntity(id, &srcRegistry);
      Ease::Entity entity = scene.Create(copyEntity.GetComponent<Component::Common>().Name());

      
      // Copy components
      CopyComponent<Component::Transform2D>(copyEntity, entity);
      CopyComponent<Component::TextRenderer2D>(copyEntity, entity);
      CopyComponent<Component::SpriteRenderer2D>(copyEntity, entity);
      CopyComponent<Component::Group>(copyEntity, entity);

      return entity;
   }
   
   Ease::Entity Scene::PasteCopiedEntity()
   {
      if(m_CopyRegistry.size() < 1)
         throw std::runtime_error("There is no copied entity");
      
      entt::entity copyEntityID = m_CopyRegistry.view<Component::Common>()[0];
      Ease::Entity newEntity = CreateEntityFromRegistry(copyEntityID, m_CopyRegistry, *this);
      return newEntity;
   }
   
   std::vector<Ease::Entity> Scene::PasteCopiedEntities()
   {
      if(m_CopyRegistry.size() < 1)
         throw std::runtime_error("There are no copied entities");
      
      std::vector<Ease::Entity> entities;
      auto view = m_CopyRegistry.view<Component::Common>();
      for(int i=0; view.size(); i++)
      {
         entities.push_back(CreateEntityFromRegistry(view[i], m_CopyRegistry, *this));
      }
      return entities;
   }
   
   uint32_t Scene::GetCopiedEntityCount()
   {
      return m_CopyRegistry.size();
   }

   Scene::Scene()
   {
      
   }

   Scene::~Scene()
   {

   }
   
   Entity Scene::Create(const std::string& name, uint32_t id /* = 0*/)
   {
      Entity entity;
      if(id == 0)
         entity.SetEntityID(m_Registry.create());
      else
         entity.SetEntityID(m_Registry.create((entt::entity)id));
      entity.SetRegistry(&m_Registry);

      auto& common = entity.AddComponent<Component::Common>();
      common.Name() = name;

      return entity;
   }
   
   void Scene::Destroy(Ease::Entity& entity)
   {
      if(m_Registry.valid((entt::entity)entity.ID()))
      {
         m_Registry.destroy((entt::entity)entity.ID());
      }
      entity.SetEntityID(entt::null);
      entity.SetRegistry(nullptr);
   }


   static void YAMLSerializeEntity(Ease::Entity entity, YAML::Emitter& yaml)
   {
      yaml << YAML::Key << entity.ID() << YAML::Value << YAML::BeginMap;
         yaml << YAML::Key << "Name" << YAML::Value << entity.GetComponent<Ease::Component::Common>().Name();

         // <Components>
         yaml << YAML::Key << "Components" << YAML::BeginMap;

            // animated sprite note for later
            // hold sequence of animation ids
            // todo: make animation a resource
            // <AnimatedSprite2D>
            if(entity.HasComponent<Component::AnimatedSprite2D>())
            {
               Component::AnimatedSprite2D& component = entity.GetComponent<Component::AnimatedSprite2D>();

               yaml << YAML::Key << "AnimatedSprite2D" << YAML::BeginMap;
                  yaml << YAML::Key << "Note" << YAML::Value << "AnimatedSprite2D not serializable right now";
               yaml << YAML::EndMap;
            }
            // </AnimatedSprite2D>
            // <Group>
            if(entity.HasComponent<Component::Group>())
            {
               Component::Group& component = entity.GetComponent<Component::Group>();

               yaml << YAML::Key << "Group" << YAML::BeginMap;
                  yaml << YAML::Key << "Groups" << YAML::Value << component.Groups();
               yaml << YAML::EndMap;
            }
            // </Group>
            // <NativeBehaviourClass>
            if(entity.HasComponent<Component::NativeBehaviourClass>())
            {
               Component::NativeBehaviourClass& component = entity.GetComponent<Component::NativeBehaviourClass>();

               yaml << YAML::Key << "NativeBehaviourClass" << YAML::BeginMap;
                  yaml << YAML::Key << "ClassName" << YAML::Value << component.ClassName();
               yaml << YAML::EndMap;
            }
            // </NativeBehaviourClass>
            // <SpriteRenderer2D>
            if(entity.HasComponent<Component::SpriteRenderer2D>())
            {
               Component::SpriteRenderer2D& component = entity.GetComponent<Component::SpriteRenderer2D>();

               yaml << YAML::Key << "SpriteRenderer2D" << YAML::BeginMap;
                  yaml << YAML::Key << "Texture" << YAML::Value << component.TextureID();
                  yaml << YAML::Key << "Visible" << YAML::Value << component.Visible();
               yaml << YAML::EndMap;
            }
            // </SpriteRenderer2D>
            // <TextRenderer2D>
            if(entity.HasComponent<Component::TextRenderer2D>())
            {
               Component::TextRenderer2D& component = entity.GetComponent<Component::TextRenderer2D>();

               yaml << YAML::Key << "TextRenderer2D" << YAML::BeginMap;
                  yaml << YAML::Key << "Text" << YAML::Value << component.Text();
                  yaml << YAML::Key << "Color" << YAML::Value << component.Color();
                  yaml << YAML::Key << "FontSize" << YAML::Value << component.FontSize();
                  yaml << YAML::Key << "Visible" << YAML::Value << component.Visible();
               yaml << YAML::EndMap;
            }
            // </TextRenderer2D>
            // <Transform2D>
            if(entity.HasComponent<Component::Transform2D>())
            {
               Component::Transform2D& component = entity.GetComponent<Component::Transform2D>();

               yaml << YAML::Key << "Transform2D" << YAML::BeginMap;
                  yaml << YAML::Key << "Position" << YAML::Value << component.Position();
                  yaml << YAML::Key << "Scale" << YAML::Value << component.Scale();
                  yaml << YAML::Key << "Rotation" << YAML::Value << component.Rotation();
                  // yaml << YAML::Key << "ZIndex" << YAML::Value << component.ZIndex();
               yaml << YAML::EndMap;
            }
            // </Transform2D>

         yaml << YAML::EndMap;
         // </Components>


         yaml << YAML::Newline;
      yaml << YAML::EndMap;
   }
   
   bool Scene::Save()
   {
      return SaveToFile(path.c_str());
   }
   bool Scene::SaveToFile(const char* file)
   {
      std::filesystem::path savepath = ProjectSettings::get_singleton().projectpath / file;
      Application::get_singleton().Log(std::string("Saving scene to ") + savepath.string());
      YAML::Emitter yaml;
      yaml << YAML::BeginMap;
         yaml << YAML::Key << "Type" << YAML::Value << "Scene";
         yaml << YAML::Newline;
         yaml << YAML::Newline;
         // <Resources>
         yaml << YAML::Key << "ResourceList" << YAML::Value;
         yaml << YAML::BeginMap;
         { // <Ease::Texture>
            yaml << YAML::Key << "Texture" << YAML::Value << YAML::BeginMap;
            ResourceManager<Ease::Texture> loader_texture = ResourceManager<Ease::Texture>::GetLoader();
            std::map<ResourceID, std::shared_ptr<Ease::Texture>> textures = loader_texture.GetResources();
            
            for(auto[id, texture] : textures)
            {
               yaml << YAML::Key << id << YAML::Value << YAML::BeginMap;
                  yaml << YAML::Key << "Path" << YAML::Value << texture->GetFilepath();
                  yaml << YAML::Newline;
               yaml << YAML::EndMap;
            }

            yaml << YAML::EndMap;
         }  // </Ease::Texture>
         yaml << YAML::Newline << YAML::EndMap;
         // </Resources>

         yaml << YAML::Key << "EntityList" << YAML::Value;
         yaml << YAML::BeginMap;
            auto view = m_Registry.view<Ease::Component::Common>();
            for(auto it = view.rbegin(); it < view.rend(); ++it)
               YAMLSerializeEntity(Ease::Entity(*it, &m_Registry), yaml);
         yaml << YAML::EndMap;
      yaml << YAML::EndMap;

      std::ofstream ofstream(savepath);
      ofstream << yaml.c_str();
      return true;
   }

   bool Scene::LoadFromFile(const char* file)
   {
      m_Registry.clear();
      std::filesystem::path inpath = ProjectSettings::get_singleton().projectpath / file;
      path = file;
      Application::get_singleton().Log(std::string("Loading scene from ") + inpath.string());

      YAML::Node node = YAML::LoadFile(inpath);
      if(node["Type"].as<std::string>("") != "Scene")
      {
         Application::get_singleton().Log(std::string("Wrong file type ") + node["Type"].as<std::string>("") + ", expected 'Scene'" );
         return false;
      }

      YAML::Node resources = node["ResourceList"];
      if(resources)
      {
         { // <Ease::Texture>
            if(YAML::Node resource_node = resources["Texture"]; resource_node)
            {
               ResourceManager<Ease::Texture>& resource_loader = ResourceManager<Ease::Texture>::GetLoader();
               for(auto it = resource_node.begin(); it != resource_node.end(); ++it)
               {
                  uint32_t id = it->first.as<uint32_t>(0);
                  YAML::Node tex_node = it->second;
                  resource_loader.LoadResource(tex_node["Path"].as<std::string>("").c_str(), id);
               }
            }
         } // </Ease::Texture>
      }
      
      YAML::Node entities = node["EntityList"];
      if(entities)
      {
         for(auto it = entities.begin(); it != entities.end(); ++it)
         {
            uint32_t id = it->first.as<uint32_t>(0);
            YAML::Node entity_node = it->second;
            std::string name = entity_node["Name"].as<std::string>("Entity");

            Ease::Entity entity = Create(name, id);
            YAML::Node components = entity_node["Components"];
            if(components)
            {
               if(YAML::Node component_node = components["AnimatedSprite2D"]; component_node)
               {
                  Component::AnimatedSprite2D& component = entity.AddComponent<Component::AnimatedSprite2D>();
                  std::cout << "AnimatedSprite2D not serializable!" << std::endl;
                  // component.() = component_node[""].as<>();
               }
               if(YAML::Node component_node = components["Group"]; component_node)
               {
                  Component::Group& component = entity.AddComponent<Component::Group>();
                  component.Groups() = component_node["Groups"].as<std::vector<std::string>>(std::vector<std::string>{});
               }
               if(YAML::Node component_node = components["NativeBehaviourClass"]; component_node)
               {
                  Component::NativeBehaviourClass& component = entity.AddComponent<Component::NativeBehaviourClass>();
                  component.ClassName() = component_node["ClassName"].as<std::string>(std::string{});
               }
               if(YAML::Node component_node = components["SpriteRenderer2D"]; component_node)
               {
                  Component::SpriteRenderer2D& component = entity.AddComponent<Component::SpriteRenderer2D>();
                  component.TextureID() = component_node["Texture"].as<ResourceID>(0);
                  component.Visible() = component_node["Visible"].as<bool>(true);
               }
               if(YAML::Node component_node = components["TextRenderer2D"]; component_node)
               {
                  Component::TextRenderer2D& component = entity.AddComponent<Component::TextRenderer2D>();
                  component.Text() = component_node["Text"].as<std::string>("");
                  component.Color() = component_node["Color"].as<glm::vec4>(glm::vec4{255.f, 255.f, 255.f, 255.f});
                  component.FontSize() = component_node["FontSize"].as<float>(64.f);
                  component.Visible() = component_node["Visible"].as<bool>(true);
                  // component.ZIndex() = component_node["ZIndex"].as<int>(0);
               }
               if(YAML::Node component_node = components["Transform2D"]; component_node)
               {
                  Component::Transform2D& component = entity.AddComponent<Component::Transform2D>();
                  component.Position() = component_node["Position"].as<glm::vec2>(glm::vec2{0.f, 0.f});
                  component.Scale() = component_node["Scale"].as<glm::vec2>(glm::vec2{1.f, 1.f});
                  component.Rotation() = component_node["Rotation"].as<float>(0.f);
                  // component.ZIndex() = component_node["ZIndex"].as<int>(0);
               }
            }
         }
      }

      return true;
   }
   
   Ease::Entity Scene::GetEntityInGroup(const std::string& group)
   {
      auto view = m_Registry.view<Component::Group>();
      for(auto it = view.rbegin(); it != view.rend(); ++it)
      {
         Ease::Entity e(*it, &m_Registry);
         
         auto groups = e.GetComponent<Component::Group>().Groups();
         for(std::string& g : groups)
         {
            if(g == group)
               return e;
         }
      }
      return Entity(entt::null, nullptr);
   }
   
   std::vector<Ease::Entity> Scene::GetEntitiesInGroup(const std::string& group)
   {
      std::vector<Ease::Entity> entities;
      auto view = m_Registry.view<Component::Group>();
      for(auto it = view.rbegin(); it != view.rend(); ++it)
      {
         Ease::Entity e(*it, &m_Registry);
         
         auto groups = e.GetComponent<Component::Group>().Groups();
         for(std::string& g : groups)
         {
            if(g == group)
               entities.push_back(e);
         }
      }
      return entities;
   }
} // namespace Ease