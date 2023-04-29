#include "yaml-cpp/yaml.h"
#include "file_buffer.hpp"

namespace sowa {
FileBuffer::FileBuffer(const FileBufferData &buf) {
	buffer = buf;
}
FileBuffer::FileBuffer(const std::string &text) {
	size_t len = text.size() + 1;
	buffer.resize(len);
	memcpy(buffer.data(), text.c_str(), len);
	buffer[len - 1] = '\n';
}
FileBuffer::FileBuffer(const YAML::Node &node) {
	YAML::Emitter e;
	e << node;
	const char *c = e.c_str();
	size_t len = strlen(c) + 1;

	buffer.resize(len);
	memcpy(buffer.data(), c, len);
	buffer[len - 1] = '\n';
}

std::string FileBuffer::String() const {
	const unsigned char *data = buffer.data();
	return std::string(reinterpret_cast<const char *>(data), buffer.size());
}

YAML::Node FileBuffer::Yaml() const {
	try {
		YAML::Node node = YAML::Load(String());
		return node;
	} catch (const std::exception &e) {
		return YAML::Node();
	}
}

// Same as Yaml() but throws exception on error
YAML::Node FileBuffer::YamlThrow() const {
	try {
		YAML::Node node = YAML::Load(String());
		return node;
	} catch (const std::exception &e) {
		throw e;
	}
}
} // namespace sowa
