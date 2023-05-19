#ifndef SW_TOML_SERIALIZER_HPP_
#define SW_TOML_SERIALIZER_HPP_

#include <string>

namespace sowa {
class SerializeDocument;
class TomlSerializer {
  public:
	static SerializeDocument Load(const std::string &path);
};
} // namespace sowa

#endif // SW_TOML_SERIALIZER_HPP_