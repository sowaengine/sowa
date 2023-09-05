#include "node_2d.hxx"

#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/quaternion.hpp"

glm::mat4 Node2D::CalculateTransform() {
	glm::mat4 model(1.f);
	if (Node *parent = GetParent(); nullptr != parent) {
		Node2D *parentNode = dynamic_cast<Node2D *>(parent);
		if (nullptr != parentNode) {
			model = parentNode->CalculateTransform();
		}
	}

	model = glm::translate(model, {m_position.x, m_position.y, 0.f});
	model = glm::rotate(model, glm::radians(m_rotation), {0.f, 0.f, 1.f});
	model = glm::scale(model, {m_scale.x, m_scale.y, 1.f});

	return model;
}

vec2 Node2D::GlobalPosition() {
	glm::vec3 pos = CalculateTransform()[3];
	return vec2(pos.x, pos.y);
}
float Node2D::GlobalRotation() {
	glm::quat rot = glm::quat_cast(CalculateTransform());
	return glm::degrees(glm::eulerAngles(rot).z);
}