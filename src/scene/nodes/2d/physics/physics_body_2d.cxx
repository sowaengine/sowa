#include "physics_body_2d.hxx"

#include "core/app.hxx"
#include "scene/nodes/2d/physics/base_collider_2d.hxx"
#include "servers/physics_server_2d.hxx"
#include "utils/utils.hxx"

void PhysicsBody2D::_start() {
	m_body = PhysicsServer2D::get().create_body(m_type, position(), rotation());

	for (Node *node : get_children()) {
		BaseCollider2D *collider = dynamic_cast<BaseCollider2D *>(node);
		if (nullptr != collider) {
			collider->body_add_fixture(m_body);
		}
	}
}

void PhysicsBody2D::_update() {

	if (App::get().IsRunning()) {
		position() = PhysicsServer2D::get().body_get_position(m_body);
		rotation() = PhysicsServer2D::get().body_get_rotation(m_body);
	}
}

void PhysicsBody2D::_exit() {
	PhysicsServer2D::get().destroy_body(m_body);
	m_body = nullptr;
}

void PhysicsBody2D::_contact_begin(uint64_t id_a, uint64_t id_b) {
	Node *node_a = App::get().GetCurrentScene()->get_node_by_id(id_a);
	Node *node_b = App::get().GetCurrentScene()->get_node_by_id(id_b);

	Utils::Log("Contact begin between {} and {}", node_a->name(), node_b->name());
}

void PhysicsBody2D::_contact_end(uint64_t id_a, uint64_t id_b) {
	Node *node_a = App::get().GetCurrentScene()->get_node_by_id(id_a);
	Node *node_b = App::get().GetCurrentScene()->get_node_by_id(id_b);

	Utils::Log("Contact end between {} and {}", node_a->name(), node_b->name());
}