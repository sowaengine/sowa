#include "math.hxx"

#include "glm/glm.hpp"

float math::sin(float f) {
	return glm::sin(f);
}

float math::cos(float f) {
	return glm::cos(f);
}

float math::pi() {
	return M_PI;
}

float math::radians(float deg) {
	return glm::radians(deg);
}

float math::degrees(float rad) {
	return glm::degrees(rad);
}

float math::atan2(float y, float x) {
	return ::atan2(y, x);
}

float math::fmod(float x, float y) {
	return ::fmod(x, y);
}