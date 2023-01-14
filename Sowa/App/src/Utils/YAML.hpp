#ifndef _E_YAML_HPP__
#define _E_YAML_HPP__

#pragma once

#include "Utils/Math.hpp"
#include "glm/glm.hpp"
#include "yaml-cpp/yaml.h"

#include <string>

namespace YAML {

template <>
struct convert<glm::vec2> {
	static Node encode(const glm::vec2 &vec) {
		Node node;
		node.push_back(vec.x);
		node.push_back(vec.y);
		return node;
	}
	static bool decode(const Node &node, glm::vec2 &vec) {
		if (!node.IsSequence() || node.size() != 2)
			return false;

		vec.x = node[0].as<float>();
		vec.y = node[1].as<float>();
		return true;
	}
};
template <>
struct convert<glm::vec3> {
	static Node encode(const glm::vec3 &vec) {
		Node node;
		node.push_back(vec.x);
		node.push_back(vec.y);
		node.push_back(vec.z);
		return node;
	}
	static bool decode(const Node &node, glm::vec3 &vec) {
		if (!node.IsSequence() || node.size() != 3)
			return false;

		vec.x = node[0].as<float>();
		vec.y = node[1].as<float>();
		vec.z = node[2].as<float>();
		return true;
	}
};
template <>
struct convert<glm::vec4> {
	static Node encode(const glm::vec4 &vec) {
		Node node;
		node.push_back(vec.x);
		node.push_back(vec.y);
		node.push_back(vec.z);
		node.push_back(vec.w);
		return node;
	}
	static bool decode(const Node &node, glm::vec4 &vec) {
		if (!node.IsSequence() || node.size() != 4)
			return false;

		vec.x = node[0].as<float>();
		vec.y = node[1].as<float>();
		vec.z = node[2].as<float>();
		vec.w = node[3].as<float>();
		return true;
	}
};

template <>
struct convert<Sowa::Vec2> {
	static Node encode(const Sowa::Vec2 &vec) {
		Node node;
		node.push_back(vec.x);
		node.push_back(vec.y);
		return node;
	}
	static bool decode(const Node &node, Sowa::Vec2 &vec) {
		if (!node.IsSequence() || node.size() != 2)
			return false;

		vec.x = node[0].as<float>();
		vec.y = node[1].as<float>();
		return true;
	}
};

YAML::Emitter &operator<<(YAML::Emitter &out, const glm::vec2 &vec);
YAML::Emitter &operator<<(YAML::Emitter &out, const glm::vec3 &vec);
YAML::Emitter &operator<<(YAML::Emitter &out, const glm::vec4 &vec);

YAML::Emitter &operator<<(YAML::Emitter &out, const Sowa::Vec2 &vec);

// YAML::Emitter &operator<<(YAML::Emitter &out, const b2Vec2 &vec);
} // namespace YAML

#endif