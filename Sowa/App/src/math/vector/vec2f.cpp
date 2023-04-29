#include "vec2f.hpp"

namespace sowa {
vec2f::vec2f(float x_, float y_)
	: x(x_), y(y_) {
	m_type = Typename();
}

vec2f::vec2f()
	: x(0.f), y(0.f) {
	m_type = Typename();
}

vec2f &vec2f::operator=(const vec2f &rhs) {
	x = rhs.x;
	y = rhs.y;

	return *this;
}

vec2f &vec2f::operator=(const glm::vec2 &rhs) {
	x = rhs.x;
	y = rhs.y;

	return *this;
}

vec2f::operator glm::vec2() {
	return glm::vec2(x, y);
}

FileBuffer vec2f::SaveImpl(object_type *obj) {
	YamlNode doc;
	vec2f *o = reinterpret_cast<vec2f *>(obj);
	doc["x"] = o->x;
	doc["y"] = o->y;
	return FileBuffer(doc);
}

bool vec2f::LoadImpl(object_type *out, const FileBuffer &buf) {
	YAML::Node doc = buf.Yaml();

	vec2f *o = reinterpret_cast<vec2f *>(out);
	o->x = doc["x"].as<int>(0);
	o->y = doc["x"].as<int>(0);
	return true;
}

vec2f &vec2f::operator=(std::initializer_list<float> rhs) {
	if (rhs.size() != 2) {
		return *this;
	}

	auto it = rhs.begin();
	x = *it;
	++it;
	y = *it;

	return *this;
}

vec2f &vec2f::operator=(std::initializer_list<double> rhs) {
	if (rhs.size() != 2) {
		return *this;
	}

	auto it = rhs.begin();
	x = *it;
	++it;
	y = *it;

	return *this;
}

vec2f &vec2f::operator=(std::initializer_list<int> rhs) {
	if (rhs.size() != 2) {
		return *this;
	}

	auto it = rhs.begin();
	x = *it;
	++it;
	y = *it;

	return *this;
}

/// @brief Returns magnitude of vector
float vec2f::length() {
	return sqrt(x * x + y * y);
}

/// @brief returns a vector with same direction with magnitude @param length
vec2f vec2f::clamp(float length /*= 1.0f*/) {
	float vecLen = this->length();
	return vec2f((x / vecLen) * length, (y / vecLen) * length);
}

/// @brief returns angle of vector in radians starting from right and rotates clockwise. (1, 0) -> 0 and (0, 1) -> PI / 2
float vec2f::angle() {
	return atan2(y, x);
}
} // namespace sowa
