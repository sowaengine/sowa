#ifndef SW_PHYSICS_SERVER_2D_HXX
#define SW_PHYSICS_SERVER_2D_HXX
#pragma once

#include "math/math.hxx"
#include "sowa.hxx"

class b2World;

enum class PhysicsBodyType {
	Static,
	Kinematic,
	Dynamic
};

std::string PhysicsBodyTypeToString(PhysicsBodyType type);
PhysicsBodyType StringToPhysicsBodyType(const std::string &type);

class PhysicsServer2D {
  public:
	static PhysicsServer2D &get();

	PhysicsServer2D() = default;
	~PhysicsServer2D();

	void init();

	inline const vec2 &get_gravity() const { return m_gravity; }
	void set_gravity(cref<vec2> gravity);

	void step();
	void debug_draw();

	void *create_body(PhysicsBodyType bodyType, cref<vec2> position, float rotation = 0.f);
	void destroy_body(void *body);
	void *body_add_box_shape(void *body, cref<vec2> halfSize, cref<vec2> position, float rotation = 0.f);
	vec2 body_get_position(void *body);
	float body_get_rotation(void *body);

  private:
	vec2 m_gravity = vec2{0.f, -9.8f};
	b2World *m_world = nullptr;
};

#endif // SW_PHYSICS_SERVER_2D_HXX