#ifndef SW_BUFFER_GL_HPP_
#define SW_BUFFER_GL_HPP_

#include <cstdint>

namespace sowa {
namespace gfx {

enum class BufferType {
	VertexBuffer = 0,
	IndexBuffer,
};

enum class BufferUsage {
	StaticDraw = 0,
	DynamicDraw,
};

class GLBuffer {
  public:
	GLBuffer() = default;

	GLBuffer(BufferType type);
    ~GLBuffer();

    void New(BufferType type);
    void Delete();

    void Bind();
    void Unbind();

    void BufferData(const void* data, uint32_t size, BufferUsage usage);
    void BufferSubdata(const void* data, uint32_t size, uint32_t offset);

  private:
	uint32_t m_id = 0;
	BufferType m_type;
};

} // namespace gfx
} // namespace sowa

#endif