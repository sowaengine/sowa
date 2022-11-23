#ifndef _E_YAML_HPP__
#define _E_YAML_HPP__

#pragma once

#include "ECS/Components/PhysicsBody2D/PhysicsBody2D.hpp"
#include "Utils/Math.hpp"
#include "box2d/box2d.h"
#include "glm/glm.hpp"
#include "imgui-docking/imgui.h"
#include "yaml-cpp/yaml.h"
#include <iostream>

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
struct convert<Ease::Vec2> {
	static Node encode(const Ease::Vec2 &vec) {
		Node node;
		node.push_back(vec.x);
		node.push_back(vec.y);
		return node;
	}
	static bool decode(const Node &node, Ease::Vec2 &vec) {
		if (!node.IsSequence() || node.size() != 2)
			return false;

		vec.x = node[0].as<float>();
		vec.y = node[1].as<float>();
		return true;
	}
};

template <>
struct convert<b2Vec2> {
	static Node encode(const b2Vec2 &vec) {
		Node node;
		node.push_back(vec.x);
		node.push_back(vec.y);
		return node;
	}
	static bool decode(const Node &node, b2Vec2 &vec) {
		if (!node.IsSequence() || node.size() != 2)
			return false;

		vec.x = node[0].as<float>();
		vec.y = node[1].as<float>();
		return true;
	}
};

template <>
struct convert<Ease::PhysicsBodyType> {
	static Node encode(const Ease::PhysicsBodyType &bodyType) {
		Node node;
		if (bodyType == Ease::PhysicsBodyType::STATIC)
			node.push_back("STATIC");
		else if (bodyType == Ease::PhysicsBodyType::DYNAMIC)
			node.push_back("DYNAMIC");
		else if (bodyType == Ease::PhysicsBodyType::KINEMATIC)
			node.push_back("KINEMATIC");
		else
			node.push_back("STATIC");
		return node;
	}
	static bool decode(const Node &node, Ease::PhysicsBodyType &bodyType) {
		if (std::string type = node.as<std::string>(std::string{""}); type != "") {
			if (type == "STATIC")
				bodyType = Ease::PhysicsBodyType::STATIC;
			else if (type == "DYNAMIC")
				bodyType = Ease::PhysicsBodyType::DYNAMIC;
			else if (type == "KINEMATIC")
				bodyType = Ease::PhysicsBodyType::KINEMATIC;
			else
				return false;
			return true;
		}
		return false;
	}
};

template <>
struct convert<Ease::ColliderShape2D> {
	static Node encode(const Ease::ColliderShape2D &shape) {
		Node node;
		if (shape == Ease::ColliderShape2D::BOX)
			node.push_back("BOX");
		else if (shape == Ease::ColliderShape2D::CIRCLE)
			node.push_back("CIRCLE");
		else
			node.push_back("BOX");
		return node;
	}
	static bool decode(const Node &node, Ease::ColliderShape2D &shape) {
		if (std::string type = node.as<std::string>(std::string{""}); type != "") {
			if (type == "BOX")
				shape = Ease::ColliderShape2D::BOX;
			else if (type == "CIRCLE")
				shape = Ease::ColliderShape2D::CIRCLE;
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

YAML::Emitter &operator<<(YAML::Emitter &out, const Ease::Vec2 &vec);

YAML::Emitter &operator<<(YAML::Emitter &out, const b2Vec2 &vec);

YAML::Emitter &operator<<(YAML::Emitter &out, const Ease::PhysicsBodyType &type);
YAML::Emitter &operator<<(YAML::Emitter &out, const Ease::ColliderShape2D &shape);
} // namespace YAML

#endif