#include "physics_body_2d.hxx"

#include "core/app.hxx"
#include "scene/nodes/2d/physics/base_collider_2d.hxx"
#include "servers/physics_server_2d.hxx"
#include "utils/utils.hxx"

PhysicsBody2D::~PhysicsBody2D() {
	destroy_body();
}

void PhysicsBody2D::_start() {
	create_body();
}

void PhysicsBody2D::_update() {

	if (App::get().IsRunning()) {

		position() = PhysicsServer2D::get().body_get_position(m_body);
		rotation() = PhysicsServer2D::get().body_get_rotation(m_body);
	}
}

void PhysicsBody2D::_exit() {
	destroy_body();
}

void PhysicsBody2D::_contact_begin(uint64_t id_a, uint64_t id_b) {
	Node *node_a = App::get().GetCurrentScene()->get_node_by_id(id_a);
	Node *node_b = App::get().GetCurrentScene()->get_node_by_id(id_b);

	for (auto &[id, b] : get_behaviours()) {
		ScriptFunctionCaller caller;
		ErrorCode err = b.CallFunc(this, "void _contact_begin(Node@, Node@)", caller);
		if (err == OK) {
			caller.arg_object(node_a).arg_object(node_b).call();
		}
	}
}

void PhysicsBody2D::_contact_end(uint64_t id_a, uint64_t id_b) {
	Node *node_a = App::get().GetCurrentScene()->get_node_by_id(id_a);
	Node *node_b = App::get().GetCurrentScene()->get_node_by_id(id_b);

	(void)node_a;
	(void)node_b;
}

void PhysicsBody2D::set_linear_velocity(vec2 velocity) {
	PhysicsServer2D::get().body_set_linear_velocity(m_body, velocity);
}

void PhysicsBody2D::_duplicate_data(Node *dst) {
	dynamic_cast<PhysicsBody2D *>(dst)->create_body();
}

void PhysicsBody2D::create_body() {
	m_body = PhysicsServer2D::get().create_body(body_type(), global_position(), global_rotation());

	for (Node *node : get_children()) {
		BaseCollider2D *collider = dynamic_cast<BaseCollider2D *>(node);
		if (nullptr != collider) {
			collider->body_add_fixture(m_body);
		}
	}
}

void PhysicsBody2D::destroy_body() {
	if (nullptr == m_body)
		return;

	PhysicsServer2D::get().destroy_body(m_body);
	m_body = nullptr;
}