#include "size.hpp"

#include "serialize/serializer.hpp"

namespace sowa {
FileBuffer size::SaveImpl(object_type *obj) {
	YamlNode doc;
	size *o = reinterpret_cast<size *>(obj);
	doc["W"] = o->w;
	doc["H"] = o->h;
	return FileBuffer(doc);
}

bool size::LoadImpl(object_type *out, const FileBuffer &buf) {
	YAML::Node doc = buf.Yaml();

	size *o = reinterpret_cast<size *>(out);
	o->w = doc["W"].as<int>(0);
	o->h = doc["H"].as<int>(0);
	return true;
}
} // namespace sowa
