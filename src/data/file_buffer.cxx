#include "file_buffer.hxx"
#include <memory.h>

file_buffer::file_buffer() {
}

file_buffer::file_buffer(const std::vector<unsigned char> &data) {
	m_buffer = data;
}

file_buffer::file_buffer(unsigned char *data, unsigned int size) {
	m_buffer.resize(size);
	memcpy(m_buffer.data(), (const void *)data, size);
}