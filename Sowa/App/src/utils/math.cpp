#include "utils/math.hpp"
#include <cmath>

#include "serialize/serializer.hpp"
#include "debug.hpp"

namespace Sowa {

FileBuffer Size::SaveImpl(ObjectType *obj) {
	YamlNode doc;
	Size* o = reinterpret_cast<Size*>(obj);
	doc["W"] = o->w;
	doc["H"] = o->h;
	return FileBuffer(doc);
}

bool Size::LoadImpl(ObjectType *out, const FileBuffer &buf) {
	YAML::Node doc = buf.Yaml();
	
	Size* o = reinterpret_cast<Size*>(out);
	o->w = doc["W"].as<int>(0);
	o->h = doc["H"].as<int>(0);
	return true;
}

} // namespace Sowa
