/**
 * @file VertexArray.hpp
 * @author Lexographics
 * @brief Container that holds VertexBuffers and IndexBuffer
 * @version 0.1
 * @date 2021-12-18
 */
#pragma once

#include "Debug.hpp"
#include <memory>
#include <vector>

#include "VertexBuffer.hpp"
#include "IndexBuffer.hpp"

#include "glad/glad.h"



namespace Ease
{

class VertexArray
{
   public:
      VertexArray();
      ~VertexArray();

      void Bind() const;
      void Unbind() const;


      /**
       * @brief Wrapper of glvertexattribpointer, automatically enables it
       * 
       * @param slot 
       * @param size 
       * @param type 
       * @param normalized 
       * @param stride 
       * @param pointer 
       */
      void SetAttribPointer(uint16_t slot, int16_t size, GLenum type, bool normalized, GLsizeiptr stride, const void* pointer);


      static std::shared_ptr<VertexArray> New();
   private:
      RendererID m_RendererID;

};
}