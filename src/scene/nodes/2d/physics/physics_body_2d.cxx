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