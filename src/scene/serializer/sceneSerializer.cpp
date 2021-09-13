#include "sceneSerializer.hpp"

#include <iostream>
#include <assert.h>
#include <sstream>
#include <fstream>

#include "yaml-cpp/yaml.h"
namespace YAML
{
   // YAML conversion overload


   // YAML Overloads
}

void serializeNode( YAML::Emitter& out, Ease::Node* node )
{
   out << YAML::Newline;

   out << YAML::BeginMap; // 1
      out << YAML::Key << "Node";
      out << YAML::Value << node->getUUID();

      {  /* Node name */
         out << YAML::Key << "Name";
         out << YAML::Value << node->getName();
      }
      
      out << YAML::Key << "Components";
      out << YAML::BeginMap; // 2
         {  /* Components */
            if(true /* node->hasComponent<Transform2D>() */)
            {
               out << YAML::Key << "Transform2D";
               out << YAML::BeginMap; // A
                  out << YAML::Key << "Position";
                  out << YAML::Value << 2;

                  out << YAML::Key << "Rotation";
                  out << YAML::Value << 2;
               out << YAML::EndMap; // A
            }
            if(true /* node->hasComponent<Sprite2D>() */)
            {
               out << YAML::Key << "Sprite2D";
               out << YAML::BeginMap; // A
                  out << YAML::Key << "Texture";
                  out << YAML::Value << 65;
               out << YAML::EndMap; // A
            }
         }
      out << YAML::EndMap; // 2

      out << YAML::Key << "Children";
      out << YAML::BeginSeq; // A
         // loop over children
         for(Ease::Node* _node : node->getChildren())
         {
            serializeNode(out, _node);
         }
      out << YAML::EndSeq; // A
   out << YAML::EndMap; // 1
}

namespace Ease
{
   bool sceneSerializer::serialize(const char* outputPath)
   {
      Node* x = new Node();
      Node* y = new Node();
      x->setName("My Root Node");
      y->setName("My Child Node");

      x->addChildren(y);

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
         out << YAML::BeginSeq; // 1
            serializeNode(out, x);
         out << YAML::EndSeq; // 1

      out << YAML::EndMap; // 1

      std::ofstream fout(outputPath);
      fout << out.c_str();

      std::cout << "Saved scene to: " << outputPath << std::endl;

      return true;
   }


   Node* sceneSerializer::deserialize(const char* inputPath)
   {
      assert(false && "Don't use yet");
      return nullptr;
   }
}