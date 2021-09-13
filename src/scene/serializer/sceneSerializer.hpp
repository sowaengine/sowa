#ifndef __SCENE_SERIALIZER_HPP__
#define __SCENE_SERIALIZER_HPP__

#include "scene/node/node.hpp"

namespace Ease
{
   class sceneSerializer
   {
      private:
         
      public:
         sceneSerializer() {}
         ~sceneSerializer() {}


         bool serialize(const char* outputPath);

         /**
          * @returns a node that contains all data about the scene,  (root node of the scene)
         */
         Node* deserialize(const char* inputPath);

   };
}

#endif // __SCENE_SERIALIZER_HPP__