#include "Utils/Math.hpp"
#include <cmath>

namespace Sowa {
float Vec2::Length() {
	return sqrt(x * x + y * y);
}

Vec2 Vec2::Clamp(float length) {
	float vecLen = Length();
	return Vec2((x / vecLen) * length, (y / vecLen) * length);
}

float Vec2::Angle() {
	return atan2(y, x);
}

} // namespace Sowa
