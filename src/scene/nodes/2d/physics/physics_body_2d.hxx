#ifndef SW_PHYSICS_BODY_2D_HXX
#define SW_PHYSICS_BODY_2D_HXX
#pragma once

#include "scene/nodes/2d/node_2d.hxx"
#include "servers/physics_server_2d.hxx"

class PhysicsBody2D : public Node2D {
  public:
	void _start() override;
	void _update() override;
	void _exit() override;

	void _contact_begin(uint64_t id_a, uint64_t id_b);
	void _contact_end(uint64_t id_a, uint64_t id_b);

	PhysicsBodyType &type() { return m_type; }

  private:
	PhysicsBodyType m_type = PhysicsBodyType::Static;

	void *m_body = nullptr;
};

#endif // SW_PHYSICS_BODY_2D_HXX