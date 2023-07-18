#ifndef SW_FILE_BUFFER_HXX
#define SW_FILE_BUFFER_HXX
#pragma once

#include <stddef.h>
#include <vector>

class file_buffer {
  public:
	file_buffer();
	file_buffer(const std::vector<unsigned char> &data);
	file_buffer(unsigned char *data, unsigned int size);

	unsigned char *data() { return m_buffer.data(); }
	size_t size() const { return m_buffer.size(); }

  private:
	friend class FileServer;

	std::vector<unsigned char> m_buffer;
};

#endif // SW_FILE_BUFFER_HXX