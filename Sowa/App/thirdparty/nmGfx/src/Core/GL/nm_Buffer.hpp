#ifndef __NM_GFX_BUFFER_HPP__
#define __NM_GFX_BUFFER_HPP__
#pragma once

#include <stdint.h>

namespace nmGfx
{
    enum class BufferType
    {
        VERTEX_BUFFER = 0,
        INDEX_BUFFER,
    };

    enum class BufferUsage
    {
        STATIC_DRAW = 0,
        DYNAMIC_DRAW,
    };

    class Buffer
    {
        public:
            Buffer() = default;

            Buffer(BufferType type);
            ~Buffer();

            void Create(BufferType type);
            void Delete();

            void BufferData(const void* data, uint32_t size, BufferUsage usage);

            void Use();
        
        private:
            unsigned int _bufferID = 0;
            BufferType _type;
    };
} // namespace nmGfx


#endif // __NM_GFX_GL_BUFFER_HPP__