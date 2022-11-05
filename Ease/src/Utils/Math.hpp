#ifndef _E_MATH_HPP__
#define _E_MATH_HPP__
#pragma once

namespace Ease {
struct Vec2 {
	float x;
	float y;

	Vec2(float _x, float _y)
		: x(_x), y(_y) {
	}

	Vec2()
		: x(0.f), y(0.f) {
	}

	/// @brief Returns magnitude of vector
	float Length();

	/// @brief returns a vector with same direction with magnitude @param length
	Vec2 Clamp(float length = 1.0f);

	/// @brief returns angle of vector in radians starting from right and rotates clockwise. (1, 0) -> 0 and (0, 1) -> PI / 2
	float Angle();
};
} // namespace Ease

#endif // _E_MATH_HPP__