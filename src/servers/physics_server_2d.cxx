#include "physics_server_2d.hxx"

#include "core/app.hxx"
#include "scene/nodes/2d/physics/physics_body_2d.hxx"
#include "scene/nodes/node.hxx"
#include "scene/scene.hxx"
#include "scene/scene_tree.hxx"
#include "utils/utils.hxx"

#include "box2d/box2d.h"

#define UNIT_TO_PX_SCALE (100)
#define PX_TO_UNIT(x) (x * (1.f / UNIT_TO_PX_SCALE))
#define UNIT_TO_PX(x) (x * UNIT_TO_PX_SCALE)

class DebugDraw : public b2Draw {
  public:
	DebugDraw() = default;

	void DrawPolygon(const b2Vec2 *vertices, int32_t vertexCount, const b2Color &color) override {
		Utils::Log("Draw Polygon");
	}
	void DrawSolidPolygon(const b2Vec2 *vertices, int32_t vertexCount, const b2Color &color) override {
		// Utils::Log("Draw Solid Polygon");
		if (vertexCount == 0)
			return;

		for (int i = 1; i < vertexCount; i++) {
			b2Vec2 v1 = vertices[i - 1];
			b2Vec2 v2 = vertices[i];

			App::get().Renderer().PushLine(vec2(UNIT_TO_PX(v1.x), UNIT_TO_PX(v1.y)), vec2(UNIT_TO_PX(v2.x), UNIT_TO_PX(v2.y)), 2.f, color.r, color.g, color.b, color.a, 10.f);
		}

		b2Vec2 v1 = vertices[vertexCount - 1];
		b2Vec2 v2 = vertices[0];

		App::get().Renderer().PushLine(vec2(UNIT_TO_PX(v1.x), UNIT_TO_PX(v1.y)), vec2(UNIT_TO_PX(v2.x), UNIT_TO_PX(v2.y)), 2.f, color.r, color.g, color.b, color.a, 10.f);
	}
	void DrawCircle(const b2Vec2 &center, float radius, const b2Color &color) override {
		Utils::Log("Draw Circle");
	}
	void DrawSolidCircle(const b2Vec2 &center, float radius, const b2Vec2 &axis, const b2Color &color) override {
		// Utils::Log("Draw Solid Circle");
	}
	void DrawSegment(const b2Vec2 &p1, const b2Vec2 &p2, const b2Color &color) override {
		Utils::Log("Draw Segment");
	}
	void DrawTransform(const b2Transform &xf) override {
		Utils::Log("Draw Transform");
	}
	void DrawPoint(const b2Vec2 &p, float size, const b2Color &color) override {
		Utils::Log("Draw Point");
	};
};

class ContactListener : public b2ContactListener {
	void BeginContact(b2Contact *contact) override {
		uint64_t id_a = ((Node *)(contact->GetFixtureA()->GetUserData().pointer))->id();
		uint64_t id_b = ((Node *)(contact->GetFixtureB()->GetUserData().pointer))->id();

		report_contact(id_a, id_a, id_b, true);
		report_contact(id_b, id_a, id_b, true);
	}
	void EndContact(b2Contact *contact) override {
		uint64_t id_a = ((Node *)(contact->GetFixtureA()->GetUserData().pointer))->id();
		uint64_t id_b = ((Node *)(contact->GetFixtureB()->GetUserData().pointer))->id();

		report_contact(id_a, id_a, id_b, false);
		report_contact(id_b, id_a, id_b, false);
	}

	void report_contact(uint64_t id, uint64_t id_a, uint64_t id_b, bool begin) {
		Scene *scene = SceneTree::get().get_scene();
		if (nullptr == scene)
			return;

		Node *node = scene->get_node_by_id(id);
		if (nullptr == node)
			return;

		PhysicsBody2D *body = dynamic_cast<PhysicsBody2D *>(node->get_parent());
		if (nullptr == body)
			return;

		if (begin)
			body->_contact_begin(id_a, id_b);
		else
			body->_contact_end(id_a, id_b);
	}
};

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

	b2Draw *debugDraw = new DebugDraw();
	debugDraw->SetFlags(b2Draw::e_shapeBit);
	m_world->SetDebugDraw(debugDraw);

	b2ContactListener *listener = new ContactListener;
	m_world->SetContactListener(listener);

	/*
	b2BodyDef groundBodyDef;
	groundBodyDef.position.Set(0.0f, -10.0f);
	b2Body *groundBody = m_world->CreateBody(&groundBodyDef);

	b2PolygonShape groundBox;
	groundBox.SetAsBox(50.0f, 10.0f);

	groundBody->CreateFixture(&groundBox, 0.0f);

	b2BodyDef bodyDef;
	bodyDef.type = b2_dynamicBody;
	bodyDef.position.Set(0.0f, 4.0f);
	body = m_world->CreateBody(&bodyDef);

	b2PolygonShape dynamicBox;
	dynamicBox.SetAsBox(1.0f, 1.0f);

	b2FixtureDef fixtureDef;
	fixtureDef.shape = &dynamicBox;
	fixtureDef.density = 1.0f;
	fixtureDef.friction = 0.3f;

	body->CreateFixture(&fixtureDef);
	*/
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

void PhysicsServer2D::debug_draw() {
	m_world->DebugDraw();
}

void *PhysicsServer2D::create_body(PhysicsBodyType bodyType, cref<vec2> position, float rotation) {
	b2BodyDef bodyDef;
	bodyDef.type = PhysicsBodyTypeTob2BodyType(bodyType);
	bodyDef.position.Set(PX_TO_UNIT(position.x), PX_TO_UNIT(position.y));
	bodyDef.angle = math::radians(rotation);

	b2Body *body = m_world->CreateBody(&bodyDef);

	return (void *)body;
}

void PhysicsServer2D::destroy_body(void *body) {
	if (nullptr == body)
		return;

	m_world->DestroyBody(reinterpret_cast<b2Body *>(body));
}

void *PhysicsServer2D::body_add_box_shape(Node *node, void *body, uint64_t id, cref<vec2> halfSize, cref<vec2> position, float rotation) {
	b2PolygonShape shape;
	shape.SetAsBox(PX_TO_UNIT(halfSize.x), PX_TO_UNIT(halfSize.y), b2Vec2(PX_TO_UNIT(position.x), PX_TO_UNIT(position.y)), math::radians(rotation));

	b2FixtureDef fixtureDef;
	fixtureDef.shape = &shape;
	fixtureDef.density = 1.0f;
	fixtureDef.friction = 0.3f;
	fixtureDef.restitution = 0.4f;
	fixtureDef.userData.pointer = (uintptr_t)node;

	b2Fixture *fixture = reinterpret_cast<b2Body *>(body)->CreateFixture(&fixtureDef);
	return (void *)fixture;
}

void *PhysicsServer2D::body_add_circle_shape(Node *node, void *body, uint64_t id, float radius, cref<vec2> position) {
	b2CircleShape shape;
	shape.m_p.Set(PX_TO_UNIT(position.x), PX_TO_UNIT(position.y));
	shape.m_radius = PX_TO_UNIT(radius);

	b2FixtureDef fixtureDef;
	fixtureDef.shape = &shape;
	fixtureDef.density = 1.0f;
	fixtureDef.friction = 0.3f;
	fixtureDef.restitution = .1f;
	fixtureDef.userData.pointer = (uintptr_t)node;

	b2Fixture *fixture = reinterpret_cast<b2Body *>(body)->CreateFixture(&fixtureDef);
	return (void *)fixture;
}

vec2 PhysicsServer2D::body_get_position(void *body) {
	if (nullptr == body) {
		return vec2(0.f, 0.f);
	}

	b2Vec2 pos = reinterpret_cast<b2Body *>(body)->GetPosition();
	return vec2(UNIT_TO_PX(pos.x), UNIT_TO_PX(pos.y));
}

float PhysicsServer2D::body_get_rotation(void *body) {
	if (nullptr == body) {
		return 0.f;
	}

	float angle = reinterpret_cast<b2Body *>(body)->GetAngle();
	return math::degrees(angle);
}

void PhysicsServer2D::body_set_linear_velocity(void *body, vec2 velocity) {
	if (nullptr == body) {
		return;
	}

	reinterpret_cast<b2Body *>(body)->SetLinearVelocity(b2Vec2(velocity.x, velocity.y));
}
