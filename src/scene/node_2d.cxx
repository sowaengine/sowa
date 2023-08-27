#include "node_2d.hxx"

#include "glm/gtc/matrix_transform.hpp"

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