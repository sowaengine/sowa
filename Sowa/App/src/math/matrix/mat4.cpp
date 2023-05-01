#include "mat4.hpp"

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"

namespace sowa {
mat4 CalculateOrtho(CalculateOrthoArgs args) {
	return glm::ortho(
		0 - (args.centerX * args.width),
		args.width - (args.centerX * args.width),
		0 - (args.centerY * args.height),
		args.height - (args.centerY * args.height),
		args.near,
		args.far);
}

mat4 CalculateModelMatrix(
	const glm::vec3 &translation,
	const glm::vec3 &rotation,
	const glm::vec3 &scale,
	const glm::vec3 &offset /*= glm::vec3(0.f, 0.f, 0.f)*/,
	const glm::mat4 &baseTransform /*= glm::mat4(1.f)*/
) {
	glm::mat4 transform = glm::translate(baseTransform, translation) *
						  glm::rotate(glm::mat4(1.f), glm::radians(rotation.y), {0.f, 1.f, 0.f}) *
						  glm::rotate(glm::mat4(1.f), glm::radians(rotation.x), {1.f, 0.f, 0.f}) *
						  glm::rotate(glm::mat4(1.f), glm::radians(rotation.z), {0.f, 0.f, 1.f}) *
						  glm::translate(glm::mat4(1.f), offset) *
						  glm::scale(glm::mat4(1.f), {scale.x, scale.y, scale.z});

	return transform;
}
} // namespace sowa
