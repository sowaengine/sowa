/**
 * @file IndexBuffer.hpp
 * @author Lexographics
 * @brief 
 * @version 0.1
 * @date 2021-12-18
 */
#pragma once

#include "Debug.hpp"
#include <memory>
#include "Renderer.hpp"

#include "glad/glad.h"

namespace Ease
{

class IndexBuffer
{
   public:
      IndexBuffer();
      ~IndexBuffer();

      void Bind() const;
      void Unbind() const;


      /**
       * @brief 
       * 
       * @param data pointer to data to buffer
       * @param dataSize size of uint32_t* data array
       * @param dataUsage GL_STATIC_DRAW, GL_DYNAMIC_DRAW etc. 
       */
      void BufferData(uint32_t* data, GLsizeiptr dataSize, u_int32_t dataUsage);


      static std::shared_ptr<IndexBuffer> New();
   private:
      RendererID m_RendererID;

};
}