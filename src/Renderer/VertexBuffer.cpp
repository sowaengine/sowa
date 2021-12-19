#include "VertexBuffer.hpp"

#include <memory>
#include "glad/glad.h"



namespace Ease
{

std::shared_ptr<VertexBuffer> VertexBuffer::New()
{
   return std::make_shared<VertexBuffer>();
}

VertexBuffer::VertexBuffer()
{
   glGenBuffers(1, &m_RendererID);
}

VertexBuffer::~VertexBuffer()
{
   glDeleteBuffers(1, &m_RendererID);
}

void VertexBuffer::Bind() const
{
   glBindBuffer(GL_ARRAY_BUFFER, m_RendererID);
}

void VertexBuffer::Unbind() const
{
   glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void VertexBuffer::BufferData(const void* data, u_int32_t dataSize, u_int32_t dataUsage)
{
   Bind();
   glBufferData(GL_ARRAY_BUFFER, dataSize, data, dataUsage);
}


   
} // namespace Ease
