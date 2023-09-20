#include "node_2d.hxx"

#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/quaternion.hpp"
#include "glm/gtx/matrix_decompose.hpp"

glm::mat4 Node2D::calculate_transform() {
	return Matrix::CalculateTransform(m_position, m_rotation, m_scale, get_parent_transform());
}

glm::mat4 Node2D::calculate_local_transform() {
	return Matrix::CalculateTransform(m_position, m_rotation, m_scale);
}

vec2 Node2D::global_position() {
	glm::vec3 pos = calculate_transform()[3];
	return vec2(pos.x, pos.y);
}
float Node2D::global_rotation() {
	glm::quat rot = glm::quat_cast(calculate_transform());
	return glm::degrees(glm::eulerAngles(rot).z);
}
vec2 Node2D::global_scale() {
	vec2 scale;
	decompose_transform(nullptr, nullptr, &scale);
	return scale;
}

void Node2D::set_global_transform(const glm::mat4 &transform) {
	// no
}

bool Node2D::decompose_transform(vec2 *position, float *rotation, vec2 *scale) {
	return Matrix::DecomposeTransform(calculate_transform(), position, rotation, scale);
}

glm::mat4 Node2D::get_parent_transform() {
	glm::mat4 model(1.f);
	if (Node *parent = get_parent(); nullptr != parent) {
		Node2D *parentNode = dynamic_cast<Node2D *>(parent);
		if (nullptr != parentNode) {
			model = parentNode->calculate_transform();
		}
	}
	return model;
}