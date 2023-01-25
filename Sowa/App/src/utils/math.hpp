#ifndef _E_MATH_HPP__
#define _E_MATH_HPP__
#pragma once

#include "stlpch.hpp"
#include <glm/glm.hpp>

namespace Sowa {

template <typename T>
struct Vec2 {
	T x;
	T y;

	Vec2(T _x, T _y)
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

	Vec2 &operator=(std::initializer_list<T> rhs) {
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
	float Length() {
		return sqrt(x * x + y * y);
	}

	/// @brief returns a vector with same direction with magnitude @param length
	T Clamp(float length = 1.0f) {
		float vecLen = Length();
		return T((x / vecLen) * length, (y / vecLen) * length);
	}

	/// @brief returns angle of vector in radians starting from right and rotates clockwise. (1, 0) -> 0 and (0, 1) -> PI / 2
	float Angle() {
		return atan2(y, x);
	}
};

typedef Vec2<float> Vector2;
typedef Vec2<double> Vector2d;

} // namespace Sowa

#endif // _E_MATH_HPP__