#include "Utils/YAML.hpp"
#include "Utils/Math.hpp"

namespace YAML {
YAML::Emitter &operator<<(YAML::Emitter &out, const ImVec2 &vec) {
	out << YAML::Flow;
	out << YAML::BeginSeq << vec.x << vec.y << YAML::EndSeq;
	return out;
}
YAML::Emitter &operator<<(YAML::Emitter &out, const ImVec4 &vec) {
	out << YAML::Flow;
	out << YAML::BeginSeq << vec.x << vec.y << vec.z << vec.w << YAML::EndSeq;
	return out;
}

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

// YAML::Emitter &operator<<(YAML::Emitter &out, const b2Vec2 &vec) {
// 	out << YAML::Flow;
// 	out << YAML::BeginSeq << vec.x << vec.y << YAML::EndSeq;
// 	return out;
// }

YAML::Emitter &operator<<(YAML::Emitter &out, const Sowa::PhysicsBodyType &type) {
	if (type == Sowa::PhysicsBodyType::STATIC)
		out << "STATIC";
	else if (type == Sowa::PhysicsBodyType::DYNAMIC)
		out << "DYNAMIC";
	else if (type == Sowa::PhysicsBodyType::KINEMATIC)
		out << "KINEMATIC";
	else
		out << "STATIC";
	return out;
}

YAML::Emitter &operator<<(YAML::Emitter &out, const Sowa::ColliderShape2D &type) {
	if (type == Sowa::ColliderShape2D::BOX)
		out << "BOX";
	else if (type == Sowa::ColliderShape2D::CIRCLE)
		out << "CIRCLE";
	else
		out << "BOX";
	return out;
}
} // namespace YAML
