#include "nm_Buffer.hpp"
#include "glad/glad.h"

namespace nmGfx
{
    static GLenum GetBufferType(BufferType type)
    {
        return  type == BufferType::VERTEX_BUFFER ? GL_ARRAY_BUFFER
              : type == BufferType::INDEX_BUFFER ? GL_ELEMENT_ARRAY_BUFFER
              : GL_NONE;
    }
    static GLenum GetBufferUsage(BufferUsage usage)
    {
        return  usage == BufferUsage::STATIC_DRAW ? GL_STATIC_DRAW
              : usage == BufferUsage::DYNAMIC_DRAW ? GL_DYNAMIC_DRAW
              : GL_NONE;
    }

    Buffer::Buffer(BufferType type)
    {
        Create(type);
    }

    void Buffer::Use()
    {
        glBindBuffer(GetBufferType(_type), _bufferID);
    }

    Buffer::~Buffer()
    {
        Delete();
    }

    void Buffer::Create(BufferType type)
    {
        if(_bufferID != 0)
            Delete();
        _type = type;
        
        glGenBuffers(1, &_bufferID);
    }

    void Buffer::Delete()
    {
        if(_bufferID != 0)
            glDeleteBuffers(1, &_bufferID);
        
        _bufferID = 0;
    }

    void Buffer::BufferData(const void* data, uint32_t size, BufferUsage usage)
    {
        glBufferData(GetBufferType(_type), size, data, GetBufferUsage(usage));
    }
} // namespace nmGfx
