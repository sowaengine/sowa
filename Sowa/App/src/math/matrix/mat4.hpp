#ifndef SW_MAT4_HPP_
#define SW_MAT4_HPP_

#include "glm/glm.hpp"

namespace sowa {
typedef glm::mat4 mat4;

struct CalculateOrthoArgs {
	float width;
	float height;
	float centerX; // 0.0 to 1.0
	float centerY; // 0.0 to 1.0
	float near;
	float far;

	CalculateOrthoArgs()
		: width(0.f),
		  height(0.f),
		  centerX(0.5f),
		  centerY(0.5f),
		  near(0.f),
		  far(10.f) {}
};

mat4 CalculateOrtho(CalculateOrthoArgs args);

mat4 CalculateModelMatrix(
	const glm::vec3 &translation,
	const glm::vec3 &rotation,
	const glm::vec3 &scale,
	const glm::vec3 &offset /*= glm::vec3(0.f, 0.f, 0.f)*/,
	const glm::mat4 &baseTransform /*= glm::mat4(1.f)*/
);

} // namespace sowa

#endif // SW_MAT4_HPP_