#ifndef _E_YAML_HPP__
#define _E_YAML_HPP__

#pragma once

#include "ECS/Components/PhysicsBody2D/PhysicsBody2D.hpp"
#include "Utils/Math.hpp"
// #include "box2d/box2d.h"
#include "glm/glm.hpp"
#include "imgui-docking/imgui.h"
#include "yaml-cpp/yaml.h"

#include <string>

namespace YAML {
template <>
struct convert<ImVec2> {
	static Node encode(const ImVec2 &vec) {
		Node node;
		node.push_back(vec.x);
		node.push_back(vec.y);
		return node;
	}
	static bool decode(const Node &node, ImVec2 &vec) {
		if (!node.IsSequence() || node.size() != 2)
			return false;

		vec.x = node[0].as<float>();
		vec.y = node[1].as<float>();
		return true;
	}
};

template <>
struct convert<ImVec4> {
	static Node encode(const ImVec4 &vec) {
		Node node;
		node.push_back(vec.x);
		node.push_back(vec.y);
		node.push_back(vec.z);
		node.push_back(vec.w);
		return node;
	}
	static bool decode(const Node &node, ImVec4 &vec) {
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

// template <>
// struct convert<b2Vec2> {
// 	static Node encode(const b2Vec2 &vec) {
// 		Node node;
// 		node.push_back(vec.x);
// 		node.push_back(vec.y);
// 		return node;
// 	}
// 	static bool decode(const Node &node, b2Vec2 &vec) {
// 		if (!node.IsSequence() || node.size() != 2)
// 			return false;
// 
// 		vec.x = node[0].as<float>();
// 		vec.y = node[1].as<float>();
// 		return true;
// 	}
// };

template <>
struct convert<Sowa::PhysicsBodyType> {
	static Node encode(const Sowa::PhysicsBodyType &bodyType) {
		Node node;
		if (bodyType == Sowa::PhysicsBodyType::STATIC)
			node.push_back("STATIC");
		else if (bodyType == Sowa::PhysicsBodyType::DYNAMIC)
			node.push_back("DYNAMIC");
		else if (bodyType == Sowa::PhysicsBodyType::KINEMATIC)
			node.push_back("KINEMATIC");
		else
			node.push_back("STATIC");
		return node;
	}
	static bool decode(const Node &node, Sowa::PhysicsBodyType &bodyType) {
		if (std::string type = node.as<std::string>(std::string{""}); type != "") {
			if (type == "STATIC")
				bodyType = Sowa::PhysicsBodyType::STATIC;
			else if (type == "DYNAMIC")
				bodyType = Sowa::PhysicsBodyType::DYNAMIC;
			else if (type == "KINEMATIC")
				bodyType = Sowa::PhysicsBodyType::KINEMATIC;
			else
				return false;
			return true;
		}
		return false;
	}
};

template <>
struct convert<Sowa::ColliderShape2D> {
	static Node encode(const Sowa::ColliderShape2D &shape) {
		Node node;
		if (shape == Sowa::ColliderShape2D::BOX)
			node.push_back("BOX");
		else if (shape == Sowa::ColliderShape2D::CIRCLE)
			node.push_back("CIRCLE");
		else
			node.push_back("BOX");
		return node;
	}
	static bool decode(const Node &node, Sowa::ColliderShape2D &shape) {
		if (std::string type = node.as<std::string>(std::string{""}); type != "") {
			if (type == "BOX")
				shape = Sowa::ColliderShape2D::BOX;
			else if (type == "CIRCLE")
				shape = Sowa::ColliderShape2D::CIRCLE;
			else
				return false;
			return true;
		}
		return false;
	}
};

YAML::Emitter &operator<<(YAML::Emitter &out, const ImVec2 &vec);
YAML::Emitter &operator<<(YAML::Emitter &out, const ImVec4 &vec);

YAML::Emitter &operator<<(YAML::Emitter &out, const glm::vec2 &vec);
YAML::Emitter &operator<<(YAML::Emitter &out, const glm::vec3 &vec);
YAML::Emitter &operator<<(YAML::Emitter &out, const glm::vec4 &vec);

YAML::Emitter &operator<<(YAML::Emitter &out, const Sowa::Vec2 &vec);

// YAML::Emitter &operator<<(YAML::Emitter &out, const b2Vec2 &vec);

YAML::Emitter &operator<<(YAML::Emitter &out, const Sowa::PhysicsBodyType &type);
YAML::Emitter &operator<<(YAML::Emitter &out, const Sowa::ColliderShape2D &shape);
} // namespace YAML

#endif