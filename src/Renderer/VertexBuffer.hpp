/**
 * @file VertexBuffer.hpp
 * @author Lexographics
 * @brief 
 * @version 0.1
 * @date 2021-12-18
 */
#pragma once

#include "Debug.hpp"
#include <memory>
#include "Renderer/Renderer.hpp"
namespace Ease
{

class VertexBuffer
{
   public:
      VertexBuffer();
      ~VertexBuffer();

      void Bind() const;
      void Unbind() const;

      /**
       * @brief 
       * 
       * @param data pointer to data to buffer
       * @param dataSize size of uint32_t* data array
       * @param dataUsage GL_STATIC_DRAW, GL_DYNAMIC_DRAW etc. 
       */
      void BufferData(const void* data, u_int32_t dataSize, u_int32_t dataUsage);


      static std::shared_ptr<VertexBuffer> New();
   private:
      RendererID m_RendererID;

};
}