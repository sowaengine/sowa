#include "VertexArray.hpp"

#include <memory>

#include "glad/glad.h"

namespace Ease
{

std::shared_ptr<VertexArray> VertexArray::New()
{
   return std::make_shared<VertexArray>();
}


VertexArray::VertexArray()
{
   glGenVertexArrays(1, &m_RendererID);
}

VertexArray::~VertexArray()
{
   glDeleteVertexArrays(1, &m_RendererID);
}


void VertexArray::Bind() const
{
   glBindVertexArray(m_RendererID);
}

void VertexArray::Unbind() const
{
   glBindVertexArray(0);
}

void VertexArray::SetAttribPointer(uint16_t slot, int16_t size, GLenum type, bool normalized, GLsizeiptr stride, const void* pointer)
{
   glEnableVertexAttribArray(slot);
   glVertexAttribPointer(slot, size, type, normalized, stride, pointer);
}
   
} // namespace Ease
