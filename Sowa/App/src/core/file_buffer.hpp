#ifndef SW_FILE_BUFFER_HPP_
#define SW_FILE_BUFFER_HPP_

#include "stlpch.hpp"
#include "yaml-cpp/yaml.h"
#include "debug.hpp"

namespace Sowa {
typedef std::vector<unsigned char> FileBufferData;

class FileBuffer {
  public:
    FileBuffer() = default;
    FileBuffer(const FileBufferData& buf) {
        buffer = buf;
    }
    FileBuffer(const std::string& text) {
        size_t len = text.size() + 1;
        buffer.resize(len);
        memcpy(buffer.data(), text.c_str(), len);
        buffer[len-1] = '\n';
    }
    FileBuffer(const YAML::Node& node) {
        YAML::Emitter e;
        e << node;
        const char* c = e.c_str();
        size_t len = strlen(c) + 1;

        buffer.resize(len);
        memcpy(buffer.data(), c, len);
        buffer[len-1] = '\n';
    }

    std::string String() const {
        const unsigned char* data = buffer.data();
        return std::string(reinterpret_cast<const char *>(data), buffer.size());
    }

    YAML::Node Yaml() const {
        try {
            YAML::Node node = YAML::Load(String());
            return node;
        } catch(const std::exception& e) {
            return YAML::Node();
        }
    }


    FileBufferData& Data() { return buffer; }

    private:
        FileBufferData buffer;
};
} // namespace Sowa

#endif