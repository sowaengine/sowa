#include "node_2d.hxx"

#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/quaternion.hpp"
#include "glm/gtx/matrix_decompose.hpp"

glm::mat4 Node2D::CalculateTransform() {
	return Matrix::CalculateTransform(m_position, m_rotation, m_scale, get_parent_transform());
}

glm::mat4 Node2D::CalculateLocalTransform() {
	return Matrix::CalculateTransform(m_position, m_rotation, m_scale);
}

vec2 Node2D::GlobalPosition() {
	glm::vec3 pos = CalculateTransform()[3];
	return vec2(pos.x, pos.y);
}
float Node2D::GlobalRotation() {
	glm::quat rot = glm::quat_cast(CalculateTransform());
	return glm::degrees(glm::eulerAngles(rot).z);
}
vec2 Node2D::GlobalScale() {
	vec2 scale;
	DecomposeTransform(nullptr, nullptr, &scale);
	return scale;
}

void Node2D::SetGlobalTransform(const glm::mat4 &transform) {
	// no
}

bool Node2D::DecomposeTransform(vec2 *position, float *rotation, vec2 *scale) {
	return Matrix::DecomposeTransform(CalculateTransform(), position, rotation, scale);
}

glm::mat4 Node2D::get_parent_transform() {
	glm::mat4 model(1.f);
	if (Node *parent = GetParent(); nullptr != parent) {
		Node2D *parentNode = dynamic_cast<Node2D *>(parent);
		if (nullptr != parentNode) {
			model = parentNode->CalculateTransform();
		}
	}
	return model;
}