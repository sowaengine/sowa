#ifndef SW_GFX_BUFFER_HXX
#define SW_GFX_BUFFER_HXX
#pragma once

#include "../gl.hxx"

namespace gfx {

namespace BufferType {
const int None = GL_NONE;
const int VertexBuffer = GL_ARRAY_BUFFER;
const int IndexBuffer = GL_ELEMENT_ARRAY_BUFFER;
} // namespace BufferType
typedef int buffertype_t;

namespace BufferUsage {
const int None = GL_NONE;
const int StaticDraw = GL_STATIC_DRAW;
const int DynamicDraw = GL_DYNAMIC_DRAW;
} // namespace BufferUsage
typedef int bufferusage_t;

class Buffer {
  public:
	// Initializes empty buffer, requires to be allocated with ::New
	Buffer() = default;
	// Calles ::New
	Buffer(buffertype_t bufferType);
	// Destructing buffer deallocates memory
	~Buffer();

	void New(buffertype_t bufferType);
	void Delete();

	void Bind();
	void Unbind();

	void BufferData(const void *data, uint32_t size, bufferusage_t usage);
	void BufferSubdata(const void *data, uint32_t size, uint32_t offset);

  private:
	uint32_t m_id = 0;
	buffertype_t m_bufferType = BufferType::None;
};

} // namespace gfx

#endif // SW_GFX_BUFFER_HXX