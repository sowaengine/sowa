#include "batch2d.hpp"

#include <array>

#include "glm/glm.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtx/rotate_vector.hpp"

#include "glad/glad.h"
#include <GLFW/glfw3.h>

#include "renderer/shader/shader.hpp"

#include "file/file.hpp"

#include "global.hpp"
// Ease::Window* windowRef; // already defined

namespace Ease
{


   struct data_BatchRenderer
   {
      Renderer::Vertex* QuadBuffer = nullptr;
      Renderer::Vertex* QuadBufferPointer = nullptr;
      
      std::array<uint32_t, Renderer::maxTextureCount> texSlots;
      uint32_t texSlotIndex = 1;
      
      uint32_t indexCount = 0;

      uint32_t drawCount = 0;
      uint32_t quadCount = 0;
   };

   static data_BatchRenderer renderer;

   std::array<Renderer::Vertex, Renderer::maxVertexCount>* verticesPointer = nullptr;

   
   Shader shaderProgram;
   

   // VertexArray, VertexBuffer, IndexBuffer are generated manually, not in class
   GLuint VertexArray;
   GLuint VertexBuffer;
   GLuint IndexBuffer;

   void Renderer::setIndexCount(int count) { renderer.indexCount = count; }
   void Renderer::setVerticesPointer( std::array<Renderer::Vertex, Renderer::maxVertexCount>* pointer )
   { verticesPointer = pointer; }


   void Renderer::Initialize()
   {
      shaderProgram.loadShader( File::getPath("res://resources/shaders/sprite2d_batch.glsl") );

      // Generate Vertex Array
      glGenVertexArrays(1, &VertexArray);
      glBindVertexArray(VertexArray);

      // Generate Vertex Buffer
      glGenBuffers(1, &VertexBuffer);
      glBindBuffer(GL_ARRAY_BUFFER, VertexBuffer);
      glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex)*maxVertexCount,
                  nullptr, GL_DYNAMIC_DRAW);
      
      // Generate Indices
      uint32_t indices[maxIndexCount];
      uint32_t offset = 0; // index of quad currently being drawn
      for(size_t i = 0; i<maxIndexCount; i+=6)
      {
         indices[i+0] = 0+offset;
         indices[i+1] = 1+offset;
         indices[i+2] = 2+offset;

         indices[i+3] = 2+offset;
         indices[i+4] = 3+offset;
         indices[i+5] = 0+offset;

         offset += 4;
      }
      // Generate Index Buffer
      glGenBuffers(1, &IndexBuffer);
      glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IndexBuffer);
      glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);




      // Attribute Layouts
      glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE,
            sizeof(Vertex), (const void*)offsetof(Vertex, Position));
      glEnableVertexAttribArray(0); // glm::vec3 Vertex.Position


      glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE,
            sizeof(Vertex), (const void*)offsetof(Vertex, Color));
      glEnableVertexAttribArray(1); // glm::vec4 Vertex.Color


      glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE,
            sizeof(Vertex), (const void*)offsetof(Vertex, texCoord));
      glEnableVertexAttribArray(2); // glm::vec2 Vertex.texCoord


      glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE,
            sizeof(Vertex), (const void*)offsetof(Vertex, texID));
      glEnableVertexAttribArray(3); // float Vertex.texID;

      /*  |                            | */
      /*  v  PART THAT CAN BE CHANGED  v */
      glUseProgram(shaderProgram.getID());
      auto loc = glGetUniformLocation(shaderProgram.getID(), "u_Textures");
      int samplers[maxTextureCount];
      for(int i=0; i<maxTextureCount; i++)
         samplers[i] = i;

      for(int i=0; i<maxTextureCount; i++)
      {
         renderer.texSlots[i] = 0;
      }
   }

   static Renderer::Vertex CreateVertex(glm::vec3 position, glm::vec4 color, glm::vec2 texCoord, float texIndex)
   {
      Renderer::Vertex vert;
      vert.Position = position;
      vert.Color = color;
      vert.texCoord = texCoord;
      vert.texID = texIndex;

      return vert;
   }

   Renderer::Vertex* Renderer::CreateQuad(Renderer::Vertex* target, glm::vec3 position,
         glm::vec4 color, glm::vec2 size, float rotation, float textureID)
   {
      if(renderer.indexCount >= Renderer::maxIndexCount || renderer.texSlotIndex >= Renderer::maxTextureCount)
      {
         Renderer::EndBatch();
         target = verticesPointer->data();
      }

      bool hasTex = false;
      float texIndex = 0.f;
      for(int i=0; i<renderer.texSlotIndex; i++)
      {
         if(renderer.texSlots[i] == textureID)
         {
            texIndex = (float)i;
            hasTex = true;
            break;
         }
      }

      if( texIndex == 0.f )
      {
         texIndex = (float)renderer.texSlotIndex;
         renderer.texSlots[renderer.texSlotIndex] = textureID;

         renderer.texSlotIndex++;
      }

      // WARNING
      float posx = position.x-size.x*0.5;
      float posy = position.y-size.y*0.5;
      double rot =  glm::radians(rotation+135); // 0
      double rot2 = glm::radians(rotation+225); // 90
      double rot3 = glm::radians(rotation+315); // 180
      double rot4 = glm::radians(rotation+45);  // 270

      float cx = posx+size.x*0.5f;
      float cy = posy+size.y*0.5f;

      float x1 = cx-(float)cos(rot )*size.x;
      float y1 = cy-(float)sin(rot )*size.y;
      float x2 = cx-(float)cos(rot2)*size.x;
      float y2 = cy-(float)sin(rot2)*size.y;
      float x3 = cx-(float)cos(rot3)*size.x;
      float y3 = cy-(float)sin(rot3)*size.y;
      float x4 = cx-(float)cos(rot4)*size.x;
      float y4 = cy-(float)sin(rot4)*size.y;


      *target = CreateVertex( glm::vec3(x4, y4, position.z), color, glm::vec2(0.0f, 0.0f), texIndex); target++; // top left
      *target = CreateVertex( glm::vec3(x1, y1, position.z), color, glm::vec2(1.0f, 0.0f), texIndex); target++; // top right
      *target = CreateVertex( glm::vec3(x2, y2, position.z), color, glm::vec2(1.0f, 1.0f), texIndex); target++;
      *target = CreateVertex( glm::vec3(x3, y3, position.z), color, glm::vec2(0.0f, 1.0f), texIndex); target++;

      renderer.indexCount += 6;
      renderer.quadCount++;

      return target;
   }


   void Renderer::EndBatch()
   {
      glBindBuffer(GL_ARRAY_BUFFER, VertexBuffer);
      glBufferSubData(GL_ARRAY_BUFFER, 0, verticesPointer->size()*sizeof(Vertex), verticesPointer->data());

      glUseProgram(shaderProgram.getID());
      glBindVertexArray(VertexArray);

      for(uint32_t i=0; i<renderer.texSlotIndex; i++)
         glBindTexture(GL_TEXTURE0+i, renderer.texSlots[i]);
      
      
      int w = 1280;
      int h = 720;
      glfwGetWindowSize(windowRef->Get(), &w, &h);
      glViewport(0, 0, w, h);

      glm::mat4 proj = glm::ortho(0.f, (float)w, (float)h, 0.f, -1.1f, 128.f);
      glm::mat4 cam = glm::mat4(1.f);
      // move camera here

      glUniformMatrix4fv( glGetUniformLocation(shaderProgram.getID(), "u_proj"),1 , GL_FALSE, glm::value_ptr(proj));
      glUniformMatrix4fv( glGetUniformLocation(shaderProgram.getID(), "u_cam"),1 , GL_FALSE, glm::value_ptr(cam));
      
      glDrawElements(GL_TRIANGLES, renderer.indexCount, GL_UNSIGNED_INT, nullptr);
      renderer.drawCount++;
      renderer.indexCount = 0;
   }

}