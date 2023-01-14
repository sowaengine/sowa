#include "Utils/YAML.hpp"
#include "Utils/Math.hpp"

namespace YAML {
YAML::Emitter &operator<<(YAML::Emitter &out, const glm::vec2 &vec) {
	out << YAML::Flow;
	out << YAML::BeginSeq << vec.x << vec.y << YAML::EndSeq;
	return out;
}
YAML::Emitter &operator<<(YAML::Emitter &out, const glm::vec3 &vec) {
	out << YAML::Flow;
	out << YAML::BeginSeq << vec.x << vec.y << vec.z << YAML::EndSeq;
	return out;
}
YAML::Emitter &operator<<(YAML::Emitter &out, const glm::vec4 &vec) {
	out << YAML::Flow;
	out << YAML::BeginSeq << vec.x << vec.y << vec.z << vec.w << YAML::EndSeq;
	return out;
}

YAML::Emitter &operator<<(YAML::Emitter &out, const Sowa::Vec2 &vec) {
	out << YAML::Flow;
	out << YAML::BeginSeq << vec.x << vec.y << YAML::EndSeq;
	return out;
}
} // namespace YAML
