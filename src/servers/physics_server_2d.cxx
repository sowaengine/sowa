#include "physics_server_2d.hxx"

#include "utils/utils.hxx"

#include "box2d/box2d.h"

#define UNIT_TO_PX_SCALE (100)
#define PX_TO_UNIT(x) (x * (1.f / UNIT_TO_PX_SCALE))
#define UNIT_TO_PX(x) (x * UNIT_TO_PX_SCALE)

std::string PhysicsBodyTypeToString(PhysicsBodyType type) {
	if (type == PhysicsBodyType::Static)
		return "Static";
	if (type == PhysicsBodyType::Kinematic)
		return "Kinematic";
	if (type == PhysicsBodyType::Dynamic)
		return "Dynamic";

	return "what?";
}

PhysicsBodyType StringToPhysicsBodyType(const std::string &type) {
	if (type == "Static")
		return PhysicsBodyType::Static;
	if (type == "Kinematic")
		return PhysicsBodyType::Kinematic;
	if (type == "Dynamic")
		return PhysicsBodyType::Dynamic;

	return PhysicsBodyType::Static;
}

static b2BodyType PhysicsBodyTypeTob2BodyType(PhysicsBodyType type) {
	if (type == PhysicsBodyType::Static)
		return b2_staticBody;
	if (type == PhysicsBodyType::Kinematic)
		return b2_kinematicBody;
	if (type == PhysicsBodyType::Dynamic)
		return b2_dynamicBody;

	return b2_staticBody;
}

PhysicsServer2D &PhysicsServer2D::get() {
	static PhysicsServer2D *server = new PhysicsServer2D;
	return *server;
}

b2Body *body;

PhysicsServer2D::~PhysicsServer2D() {
	delete m_world;
}

void PhysicsServer2D::init() {
	m_world = new b2World(b2Vec2(m_gravity.x, m_gravity.y));
}

void PhysicsServer2D::set_gravity(cref<vec2> gravity) {
	m_gravity = gravity;
	m_world->SetGravity(b2Vec2(m_gravity.x, m_gravity.y));
}

void PhysicsServer2D::step() {
	float timeStep = 1.0f / 60.0f;
	int32 velocityIterations = 6;
	int32 positionIterations = 2;

	m_world->Step(timeStep, velocityIterations, positionIterations);
}

void *PhysicsServer2D::create_body(cref<vec2> position, PhysicsBodyType bodyType) {
	b2BodyDef bodyDef;
	bodyDef.position.Set(PX_TO_UNIT(position.x), PX_TO_UNIT(position.y));
	bodyDef.type = PhysicsBodyTypeTob2BodyType(bodyType);

	b2Body *body = m_world->CreateBody(&bodyDef);

	return (void *)body;
}

void PhysicsServer2D::destroy_body(void *body) {
	m_world->DestroyBody(reinterpret_cast<b2Body *>(body));
}

void PhysicsServer2D::body_add_box_shape(void *body, cref<vec2> halfSize) {
	b2PolygonShape shape;
	shape.SetAsBox(PX_TO_UNIT(halfSize.x), PX_TO_UNIT(halfSize.y));

	b2FixtureDef fixtureDef;
	fixtureDef.shape = &shape;
	fixtureDef.density = 1.0f;
	fixtureDef.friction = 0.3f;

	reinterpret_cast<b2Body *>(body)->CreateFixture(&fixtureDef);
}

vec2 PhysicsServer2D::body_get_position(void *body) {
	b2Vec2 pos = reinterpret_cast<b2Body *>(body)->GetPosition();
	return vec2(UNIT_TO_PX(pos.x), UNIT_TO_PX(pos.y));
}