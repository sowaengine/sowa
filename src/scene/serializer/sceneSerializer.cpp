#include "sceneSerializer.hpp"

#include <iostream>
#include <assert.h>
#include <sstream>
#include <fstream>

#include <unordered_map>

#include "scene/components/components.hpp"

#include "global.hpp"
Ease::ResourceManager* resourceManager;

#include "debug/debug.hpp"

#include "yaml-cpp/yaml.h"
namespace YAML
{
   // YAML conversion overload
   template<>
   struct convert<glm::vec2>
   {
      static Node encode(const glm::vec2& item)
      {
         Node node;
         node.push_back(item.x);
         node.push_back(item.y);
         return node;
      }
      static bool decode(const Node& node, glm::vec2& item)
      {
         if(!node.IsSequence() || node.size() != 2)
            return false;
         
         item.x = node[0].as<float>();
         item.y = node[1].as<float>();

         return true;
      }
   };

   // YAML Overloads

   YAML::Emitter& operator<<(YAML::Emitter& out, const glm::vec2& item)
   {
      out << YAML::Flow;
      out << YAML::BeginSeq << item.x << item.y << YAML::EndSeq;
      return out;
   }
}



void serializeNode( YAML::Emitter& out, Ease::Node* node)
{
   //out << YAML::Newline;


   out << YAML::Key << "Node";
   out << YAML::BeginMap; // 1
      out << YAML::Key << "ID";
      out << YAML::Value << node->getUUID();

      {  /* Node name */
         out << YAML::Key << "Name";
         out << YAML::Value << node->getName();
      }
      
      out << YAML::Key << "Components";
      out << YAML::BeginMap; // 2
         {  /* Components */
            if(node->hasComponent<Ease::Comp::Transform2D>() )
            {
               auto& transform2d = node->getComponent<Ease::Comp::Transform2D>();

               out << YAML::Key << "Transform2D";
               out << YAML::BeginMap; // A
                  out << YAML::Key << "Position";
                  out << YAML::Value << transform2d.position;

                  out << YAML::Key << "Rotation";
                  out << YAML::Value << transform2d.rotation;

                  out << YAML::Key << "Z-Index";
                  out << YAML::Value << transform2d.zIndex;
               out << YAML::EndMap; // A
            }
            if(node->hasComponent<Ease::Comp::Sprite2D>() )
            {
               auto& sprite2d = node->getComponent<Ease::Comp::Sprite2D>();

               out << YAML::Key << "Sprite2D";
               out << YAML::BeginMap; // A
                  out << YAML::Key << "Texture";
                  out << YAML::Value << sprite2d.texture->getUUID();
               out << YAML::EndMap; // A
            }
         }
      out << YAML::EndMap; // 2

      out << YAML::Key << "Children";
      out << YAML::BeginMap; // A
         // loop over children
         for(Ease::Node* _node : node->getChildren())
         {
            serializeNode(out, _node);
         }
      out << YAML::EndMap; // A
   out << YAML::EndMap; // 1
}

Ease::Node* deserializeNode( YAML::Node yaml_node )
{
   Ease::Node* node = new Ease::Node();

   if( yaml_node["ID"] ) {
      unsigned int uuid = yaml_node["ID"].as<unsigned int>(0);
      if(uuid != 0)
         node->setUUID(uuid);
   }
   if( yaml_node["Name"] ) {
      node->setName( yaml_node["Name"].as<std::string>().c_str() );
   }

   if( yaml_node["Components"] )
   {
      YAML::Node components = yaml_node["Components"];
      if( components["Transform2D"] ) {
         node->addComponent<Ease::Comp::Transform2D>();
         auto& transform2d = node->getComponent<Ease::Comp::Transform2D>();
         transform2d.position = components["Transform2D"]["Position"].as<glm::vec2>();
         
         transform2d.rotation = components["Transform2D"]["Rotation"].as<float>();

         
         /*std::cout << "\nFound a Transform2D;"
         << "\n\t" << "Name: " << node->getName()
         << "\n\t" << "Position: [" << transform2d.position.x << "," << transform2d.position.y << "]"
         << "\n\t" << "Rotation: " << transform2d.rotation
         << "\n\t" << "Z-Index: " << transform2d.zIndex << std::endl;*/
      }

      if( components["Sprite2D"] ) {
         node->addComponent<Ease::Comp::Sprite2D>();
         auto& sprite2d = node->getComponent<Ease::Comp::Sprite2D>();
         
         //resourceManager->getTexture(components["Sprite2D"][]);
         /***
          * ! COME BACK HERE LATER
          **/
      }
   }


   if( yaml_node["Children"] )
   {
      if( yaml_node["Children"].size() > 0 )
      {
         for (YAML::const_iterator it = yaml_node["Children"].begin(); it != yaml_node["Children"].end(); ++it){
            // std::cout << "Serializing Child Node: " << it->first.as<std::string>() << "\n";
            deserializeNode(it->second);
         }
      }
   }

   return node;
}


namespace Ease
{
   bool sceneSerializer::serialize(const char* outputPath)
   {
      Node* x = new Node();
      Node* y = new Node();
      Node* z = new Node();
      Node* t = new Node();
      RootNode = x;
      x->setName("My Root Node");
      y->setName("My Child Node");
      z->setName("Second Child Node");
      t->setName("Child of second");

      x->addChildren(y);
      x->addChildren(z);
      y->addChildren(t);

      x->addComponent<Comp::Transform2D>();
      y->addComponent<Comp::Transform2D>();
      z->addComponent<Comp::Transform2D>();
      t->addComponent<Comp::Transform2D>();

      y->addComponent<Comp::Sprite2D>();
      auto& spr = y->getComponent<Comp::Sprite2D>();
      spr.texture = 
      resourceManager->getTexture( resourceManager->addTexture("filepath.png")
      );

      YAML::Emitter out;

      out << YAML::Comment("Ease scene version: 1");
      out << YAML::Newline;

      out << YAML::BeginMap; // 1
         
         out << YAML::Key << "Assets";
         out << YAML::BeginMap; // 2
            out << YAML::Key << "Textures";
            out << YAML::BeginMap; // 3
               out << YAML::Key << 65;
               out << YAML::BeginMap; // 4
                  out << YAML::Key << "Path";
                  out << YAML::Value << "path/to/texture.png";

                  out << YAML::Key << "min-filter";
                  out << YAML::Value << "REPEAT";
               out << YAML::EndMap; // 4
            out << YAML::EndMap; // 3
         out << YAML::EndMap; // 2

         out << YAML::Newline;
         out << YAML::Newline;
         out << YAML::Newline;

         out << YAML::Key << "Scene";
         out << YAML::BeginMap; // 1
            serializeNode(out, RootNode);
         out << YAML::EndMap; // 1

      out << YAML::EndMap; // 1

      std::ofstream fout(outputPath);
      fout << out.c_str();

      std::cout << "Saved scene to: " << outputPath << std::endl;

      return true;
   }


   // String representations of TextureProperties items
   const std::unordered_map<std::string, TextureProperties> TexPropToString = {
      { "REPEAT"       , TextureProperties::REPEAT        },
      { "CLAMP_TO_EDGE", TextureProperties::CLAMP_TO_EDGE },
      { "LINEAR"       , TextureProperties::LINEAR },
      { "CLAMP"        , TextureProperties::CLAMP },
   };
   TextureProperties getTextureProp(const std::string& str)
   {
      auto it = TexPropToString.find(str);
      if(it != TexPropToString.end()) {
         return it->second;
      }
      
      return TextureProperties::NONE;
   }


   Node* sceneSerializer::deserialize(const char* inputPath)
   {
      Ease::Node::resetScene();

      std::ifstream stream(inputPath);
      std::stringstream strStream;
      strStream << stream.rdbuf();

      YAML::Node data = YAML::Load(strStream.str());
      
      // Load all assets from the scene
      if(data["Assets"])
      {
         // Textures
         if(data["Assets"]["Textures"])
         {
            if( data["Assets"]["Textures"].size() > 0 )
            {
               for (YAML::const_iterator it = data["Assets"]["Textures"].begin(); it != data["Assets"]["Textures"].end(); ++it){
                  YAML::Node tex = it->second;
                  unsigned int uuid = it->first.as<unsigned int>();
                  std::string filepath = "";
                  if(tex["Path"])
                     filepath = tex["Path"].as<std::string>();


                  resourceManager->addTexture(uuid, filepath.c_str());

                  Texture* texture = resourceManager->getTexture(uuid);

                  if(tex["min-filter"]){
                     TextureProperties prop = getTextureProp( tex["min-filter"].as<std::string>() );;
                     if(prop != TextureProperties::NONE)
                        texture->min_filter = prop;
                  }


                  // load texture after setting up parameters
                  resourceManager->loadTexture(uuid);
               }
            }
         }
      }


      // if scene has 'Scene' section which has nodes in it
      if(data["Scene"])
      {
         if(data["Scene"]["Node"])
         {
            RootNode = deserializeNode( data["Scene"]["Node"] );

         } else {
            debug::logError("Scene doesnt have a root!", 0);
         }

      }


      return nullptr;
   }
}