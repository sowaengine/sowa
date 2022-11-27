#ifndef _E_MATH_HPP__
#define _E_MATH_HPP__
#pragma once

#include <cassert>
#include <glm/glm.hpp>
#include <initializer_list>

namespace Sowa {
struct Vec2 {
	float x;
	float y;

	Vec2(float _x, float _y)
		: x(_x), y(_y) {
	}

	Vec2()
		: x(0.f), y(0.f) {
	}

	Vec2 &operator=(const Vec2 &rhs) {
		x = rhs.x;
		y = rhs.y;

		return *this;
	}

	Vec2 &operator=(const glm::vec2 &rhs) {
		x = rhs.x;
		y = rhs.y;

		return *this;
	}

	operator glm::vec2() {
		return glm::vec2(x, y);
	}

	Vec2 &operator=(std::initializer_list<float> rhs) {
		if (rhs.size() != 2) {
			assert(false && "Sowa::Vec2 initializer list must have 2 float elements");
			return *this;
		}

		auto it = rhs.begin();
		x = *it;
		++it;
		y = *it;

		return *this;
	}

	/// @brief Returns magnitude of vector
	float Length();

	/// @brief returns a vector with same direction with magnitude @param length
	Vec2 Clamp(float length = 1.0f);

	/// @brief returns angle of vector in radians starting from right and rotates clockwise. (1, 0) -> 0 and (0, 1) -> PI / 2
	float Angle();
};
} // namespace Sowa

#endif // _E_MATH_HPP__