#ifndef SW_FILE_BUFFER_HPP_
#define SW_FILE_BUFFER_HPP_

#include "stlpch.hpp"
#include "debug.hpp"

namespace YAML {
    class Node;
};

namespace sowa {
typedef std::vector<unsigned char> FileBufferData;

class FileBuffer {
  public:
    FileBuffer() = default;
    FileBuffer(const FileBufferData& buf);
    FileBuffer(const std::string& text);
    FileBuffer(const YAML::Node& node);

    std::string String() const;

    YAML::Node Yaml() const;

    // Same as Yaml() but throws exception on error
    YAML::Node YamlThrow() const;

    FileBufferData& Data() { return buffer; }
    const FileBufferData& Data() const { return buffer; }

    private:
        FileBufferData buffer;
};
} // namespace sowa

#endif