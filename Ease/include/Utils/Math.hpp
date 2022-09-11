#ifndef _E_MATH_HPP__
#define _E_MATH_HPP__
#pragma once

namespace Ease {
struct Vec2 {
	float x;
	float y;

	Vec2(float x, float y)
		: x(x), y(y) {
	}

	Vec2()
		: x(0.f), y(0.f) {
	}
};
} // namespace Ease

#endif // _E_MATH_HPP__