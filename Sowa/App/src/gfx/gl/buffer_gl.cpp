#include "buffer_gl.hpp"

#include "gfx/gl/glfuncs.hpp"
#include "debug.hpp"

namespace sowa {
namespace gfx {
GLBuffer::GLBuffer(BufferType type) {
    New(type);
}
GLBuffer::~GLBuffer() {
    Delete();
}

void GLBuffer::New(BufferType type) {
    if(m_id != 0) {
        Delete();
    }
    m_type = type;
    GL().genBuffer(m_id);
}
void GLBuffer::Delete() {
    if(m_id != 0) {
        GL().deleteBuffer(m_id);
    }

    m_id = 0;
}

void GLBuffer::Bind() {
    GL().bindBuffer(m_type, m_id);
}
void GLBuffer::Unbind() {
    GL().bindBuffer(m_type, 0);
}

void GLBuffer::BufferData(const void *data, uint32_t size, BufferUsage usage) {
    GL().bufferData(m_type, data, size, usage);
}
void GLBuffer::BufferSubdata(const void *data, uint32_t size, uint32_t offset) {
    GL().bufferSubdata(m_type, data, size, offset);
}
} // namespace gfx

} // namespace sowa
