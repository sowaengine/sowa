#include "buffer.hxx"

namespace gfx {
Buffer::Buffer(buffertype_t bufferType) {
	New(bufferType);
}
Buffer::~Buffer() {
	Delete();
}

void Buffer::New(buffertype_t bufferType) {
	Delete();

	m_bufferType = bufferType;
	glGenBuffers(1, &m_id);
}

void Buffer::Delete() {
	if (m_id != 0) {
		glDeleteBuffers(1, &m_id);
	}

	m_id = 0;
}

void Buffer::Bind() {
	glBindBuffer(m_bufferType, m_id);
}
void Buffer::Unbind() {
	glBindBuffer(m_bufferType, 0);
}

void Buffer::BufferData(const void *data, uint32_t size, bufferusage_t usage) {
	glBufferData(m_bufferType, size, data, usage);
}
void Buffer::BufferSubdata(const void *data, uint32_t size, uint32_t offset) {
	glBufferSubData(m_bufferType, offset, size, data);
}
} // namespace gfx
