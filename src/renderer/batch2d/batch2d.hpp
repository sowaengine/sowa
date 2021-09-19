#ifndef __BATCH_2D_HPP__
#define __BATCH_2D_HPP__

#include "glm/glm.hpp"
#include <array>

namespace Ease
{
   namespace Renderer
   {

      static const size_t maxQuadCount    = 100;
      static const size_t maxVertexCount  = maxQuadCount*4;
      static const size_t maxIndexCount   = maxQuadCount*6;
      static const size_t maxTextureCount = 8;

      struct Vertex
      {
         glm::vec3 Position;
         glm::vec4 Color;
         glm::vec2 texCoord;
         float texID;
      };



      void Initialize();

      void EndBatch();

      void setIndexCount(int count);
      void setVerticesPointer( std::array<Renderer::Vertex, maxVertexCount>* pointer );

      Vertex* CreateQuad(Vertex* target, glm::vec3 position,
         glm::vec4 color, glm::vec2 size, float rotation, float textureID);
   }
}

#endif // __BATCH_2D_HPP__