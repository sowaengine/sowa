#include "IndexBuffer.hpp"

#include <memory>
#include "glad/glad.h"


namespace Ease
{

std::shared_ptr<IndexBuffer> IndexBuffer::New()
{
   return std::make_shared<IndexBuffer>();
}

IndexBuffer::IndexBuffer()
{
   glGenBuffers(1, &m_RendererID);
}

IndexBuffer::~IndexBuffer()
{
   glDeleteBuffers(1, &m_RendererID);
}


void IndexBuffer::Bind() const
{
   glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_RendererID);
}

void IndexBuffer::Unbind() const
{
   glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void IndexBuffer::BufferData(uint32_t* data, GLsizeiptr dataSize, u_int32_t dataUsage)
{
   Bind();
   glBufferData(GL_ELEMENT_ARRAY_BUFFER, dataSize, data, dataUsage);
}



} // namespace Ease
