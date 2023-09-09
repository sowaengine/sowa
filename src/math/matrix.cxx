#include "matrix.hxx"
#include "vec2.hxx"

#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/quaternion.hpp"
#include "glm/gtx/matrix_decompose.hpp"

glm::mat4 Matrix::CalculateTransform(vec2 position, float rotation, vec2 scale, const glm::mat4 &base) {
	glm::mat4 transform = glm::translate(base, {position.x, position.y, 0.f});
	transform = glm::rotate(transform, glm::radians(rotation), {0.f, 0.f, 1.f});
	transform = glm::scale(transform, {scale.x, scale.y, 1.f});

	return transform;
}

bool Matrix::DecomposeTransform(const glm::mat4 &mat, vec2 *position, float *rotation, vec2 *scale) {
	glm::vec3 _position;
	glm::vec3 _scale;
	glm::quat _orientation;
	glm::vec3 _skew;
	glm::vec4 _perspective;

	bool ok = glm::decompose(mat, _scale, _orientation, _position, _skew, _perspective);
	if (!ok)
		return false;

	if (position)
		*position = vec2(_position.x, _position.y);
	if (rotation)
		*rotation = glm::degrees(glm::eulerAngles(_orientation).z);
	if (scale)
		*scale = vec2(_scale.x, _scale.y);

	return true;
}