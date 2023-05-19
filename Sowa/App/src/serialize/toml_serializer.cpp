#include "toml_serializer.hpp"
#include "document.hpp"

#include "utils/file.hpp"

namespace sowa {
SerializeDocument TomlSerializer::Load(const std::string &path) {
	FileBuffer buf = File::LoadFile(File::Path(path));

	toml::parse_result res = toml::parse(buf.String());
	return SerializeDocument(res);
}
} // namespace sowa
