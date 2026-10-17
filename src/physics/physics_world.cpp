#include "physics/physics_world.hpp"

#include <algorithm>
#include <cmath>

#include "core/log.hpp"
#include "core/profiler.hpp"
#include "ecs/components/transform2d.hpp"
#include "ecs/world.hpp"
#include "math/math.hpp"
#include "math/matrix.hpp"

namespace sowa {
namespace {

constexpr f32 kDegToRad = Math::Pi / 180.f;
constexpr f32 kRadToDeg = 180.f / Math::Pi;

b2BodyType ToB2Type(BodyType2D type) {
	switch (type) {
	case BodyType2D::Static:
		return b2_staticBody;
	case BodyType2D::Kinematic:
		return b2_kinematicBody;
	case BodyType2D::Dynamic:
		return b2_dynamicBody;
	}
	return b2_dynamicBody;
}

bool SameBody(b2BodyId a, b2BodyId b) {
	return a.index1 == b.index1 && a.world0 == b.world0 && a.generation == b.generation;
}

Vector2 RotatePx(Vector2 v, f32 degrees) {
	const f32 r = degrees * kDegToRad;
	const f32 c = Math::Cos(r);
	const f32 s = Math::Sin(r);
	return Vector2(c * v.x - s * v.y, s * v.x + c * v.y);
}

struct DebugDrawState {
	f32 pixelsPerMeter = 100.f;
	const std::function<void(Vector2, Vector2, Color)> *drawLine = nullptr;

	Vector2 px(b2Vec2 meters) const {
		return Vector2(meters.x * pixelsPerMeter, meters.y * pixelsPerMeter);
	}

	Color color(b2HexColor hex) const {
		const u32 v = static_cast<u32>(hex);
		return Color::RGB(static_cast<u8>((v >> 16) & 0xFF), static_cast<u8>((v >> 8) & 0xFF),
			static_cast<u8>(v & 0xFF));
	}

	void line(Vector2 a, Vector2 b, Color c) const {
		if (drawLine != nullptr && *drawLine) {
			(*drawLine)(a, b, c);
		}
	}

	void lineMeters(b2Vec2 a, b2Vec2 b, b2HexColor hex) const {
		line(px(a), px(b), color(hex));
	}

	void circle(b2Vec2 center, f32 radiusMeters, b2HexColor hex) const {
		constexpr i32 kSegments = 16;
		const Vector2 c = px(center);
		const f32 r = radiusMeters * pixelsPerMeter;
		Vector2 prev = c + Vector2(r, 0.f);
		for (i32 i = 1; i <= kSegments; ++i) {
			const f32 angle = (static_cast<f32>(i) / static_cast<f32>(kSegments)) * 2.f * Math::Pi;
			const Vector2 p = c + Vector2(Math::Cos(angle) * r, Math::Sin(angle) * r);
			line(prev, p, color(hex));
			prev = p;
		}
	}
};

void DrawPolygon(const b2Vec2 *vertices, i32 vertexCount, b2HexColor hex, void *context) {
	auto *state = static_cast<DebugDrawState *>(context);
	if (state == nullptr || vertices == nullptr || vertexCount < 2) {
		return;
	}
	for (i32 i = 0; i < vertexCount; ++i) {
		state->lineMeters(vertices[i], vertices[(i + 1) % vertexCount], hex);
	}
}

void DrawSolidPolygon(b2Transform transform, const b2Vec2 *vertices, i32 vertexCount, f32,
	b2HexColor hex, void *context) {
	auto *state = static_cast<DebugDrawState *>(context);
	if (state == nullptr || vertices == nullptr || vertexCount < 2) {
		return;
	}
	for (i32 i = 0; i < vertexCount; ++i) {
		const b2Vec2 a = b2TransformPoint(transform, vertices[i]);
		const b2Vec2 b = b2TransformPoint(transform, vertices[(i + 1) % vertexCount]);
		state->lineMeters(a, b, hex);
	}
}

void DrawCircle(b2Vec2 center, f32 radius, b2HexColor hex, void *context) {
	auto *state = static_cast<DebugDrawState *>(context);
	if (state == nullptr) {
		return;
	}
	state->circle(center, radius, hex);
}

void DrawSolidCircle(b2Transform transform, f32 radius, b2HexColor hex, void *context) {
	auto *state = static_cast<DebugDrawState *>(context);
	if (state == nullptr) {
		return;
	}
	state->circle(transform.p, radius, hex);
	const b2Vec2 axis = b2RotateVector(transform.q, b2Vec2{radius, 0.f});
	state->lineMeters(transform.p, b2Vec2{transform.p.x + axis.x, transform.p.y + axis.y}, hex);
}

void DrawSolidCapsule(b2Vec2 p1, b2Vec2 p2, f32 radius, b2HexColor hex, void *context) {
	auto *state = static_cast<DebugDrawState *>(context);
	if (state == nullptr) {
		return;
	}
	state->circle(p1, radius, hex);
	state->circle(p2, radius, hex);
	state->lineMeters(p1, p2, hex);
}

void DrawSegment(b2Vec2 p1, b2Vec2 p2, b2HexColor hex, void *context) {
	auto *state = static_cast<DebugDrawState *>(context);
	if (state == nullptr) {
		return;
	}
	state->lineMeters(p1, p2, hex);
}

void DrawTransform(b2Transform transform, void *context) {
	auto *state = static_cast<DebugDrawState *>(context);
	if (state == nullptr) {
		return;
	}
	constexpr f32 kAxis = 0.25f;
	const b2Vec2 x = b2RotateVector(transform.q, b2Vec2{kAxis, 0.f});
	const b2Vec2 y = b2RotateVector(transform.q, b2Vec2{0.f, kAxis});
	state->lineMeters(transform.p, b2Vec2{transform.p.x + x.x, transform.p.y + x.y},
		static_cast<b2HexColor>(0xFF0000));
	state->lineMeters(transform.p, b2Vec2{transform.p.x + y.x, transform.p.y + y.y},
		static_cast<b2HexColor>(0x00FF00));
}

void DrawPoint(b2Vec2 p, f32 size, b2HexColor hex, void *context) {
	auto *state = static_cast<DebugDrawState *>(context);
	if (state == nullptr) {
		return;
	}
	const Vector2 c = state->px(p);
	f32 half = size * state->pixelsPerMeter * 0.5f;
	if (half < 2.f) {
		half = 2.f;
	}
	state->line(Vector2(c.x - half, c.y), Vector2(c.x + half, c.y), state->color(hex));
	state->line(Vector2(c.x, c.y - half), Vector2(c.x, c.y + half), state->color(hex));
}

void DrawString(b2Vec2, const char *, b2HexColor, void *) {
}

} // namespace

PhysicsWorld::~PhysicsWorld() {
	Shutdown();
}

void PhysicsWorld::Init(const PhysicsWorldConfig &config) {
	createWorld(config);
}

void PhysicsWorld::Init(World &world, const PhysicsWorldConfig &config) {
	createWorld(config);
	bindWorld(world);
}

void PhysicsWorld::createWorld(const PhysicsWorldConfig &config) {
	Shutdown();
	_config = config;
	if (_config.fixedDelta <= 0.f) {
		_config.fixedDelta = 1.f / 60.f;
	}
	if (_config.subStepCount < 1) {
		_config.subStepCount = 1;
	}
	if (_config.maxStepsPerFrame < 1) {
		_config.maxStepsPerFrame = 1;
	}
	if (_config.pixelsPerMeter <= 0.f) {
		_config.pixelsPerMeter = 100.f;
	}

	b2WorldDef def = b2DefaultWorldDef();
	def.gravity = b2Vec2{_config.gravity.x, _config.gravity.y};
	_worldId = b2CreateWorld(&def);
	_accumulator = 0.f;
}

void PhysicsWorld::bindWorld(World &world) {
	_ecsWorld = &world;
	if (!world.FindComponent("RigidBody2D").IsValid()) {
		world.RegisterComponent<RigidBody2D>("RigidBody2D")
			.Field("type", &RigidBody2D::type)
			.Field("gravityScale", &RigidBody2D::gravityScale)
			.Field("linearDamping", &RigidBody2D::linearDamping)
			.Field("angularDamping", &RigidBody2D::angularDamping)
			.Field("fixedRotation", &RigidBody2D::fixedRotation);
	}
	if (!world.FindComponent("Collider2D").IsValid()) {
		world.RegisterComponent<Collider2D>("Collider2D")
			.Field("type", &Collider2D::type)
			.Field("size", &Collider2D::size)
			.Field("radius", &Collider2D::radius)
			.Field("offset", &Collider2D::offset)
			.Field("rotation", &Collider2D::rotation)
			.Field("isTrigger", &Collider2D::isTrigger)
			.Field("density", &Collider2D::density)
			.Field("friction", &Collider2D::friction)
			.Field("restitution", &Collider2D::restitution)
			.Field("collisionLayer", &Collider2D::collisionLayer)
			.Field("collisionMask", &Collider2D::collisionMask);
	}
	if (!world.FindComponent("Raycast2D").IsValid()) {
		world.RegisterComponent<Raycast2D>("Raycast2D")
			.Field("enabled", &Raycast2D::enabled)
			.Field("targetPosition", &Raycast2D::targetPosition)
			.Field("collisionLayer", &Raycast2D::collisionLayer)
			.Field("collisionMask", &Raycast2D::collisionMask)
			.Field("includeSensors", &Raycast2D::includeSensors)
			.Field("excludeOwner", &Raycast2D::excludeOwner);
	}
	_colliderRemoved = world.Flecs()
						   .observer<Collider2D>()
						   .event(flecs::OnRemove)
						   .each([this](flecs::entity entity, Collider2D &) {
							   if (auto it = _shapes.find(Entity(entity)); it != _shapes.end()) {
								   destroyShape(it->first, it->second);
								   _shapes.erase(it);
							   }
						   });
	_bodyRemoved = world.Flecs()
					   .observer<RigidBody2D>()
					   .event(flecs::OnRemove)
					   .each([this](flecs::entity entity, RigidBody2D &) {
						   if (auto it = _bodies.find(Entity(entity)); it != _bodies.end()) {
							   destroyBody(it->first, it->second);
							   _bodies.erase(it);
						   }
					   });
	_raycastRemoved =
		world.Flecs()
			.observer<Raycast2D>()
			.event(flecs::OnRemove)
			.each([this](flecs::entity entity, Raycast2D &) { _raycasts.erase(Entity(entity)); });
	_raycastsQuery = world.CreateQuery<Raycast2D>();
	_bodiesQuery = world.CreateQuery<RigidBody2D>();
	_collidersQuery = world.CreateQuery<Collider2D>();
	registerStepSystem(world);
}

void PhysicsWorld::registerStepSystem(World &world) {
	flecs::system system =
		world.Flecs().system("PhysicsStep").kind(flecs::OnValidate).immediate().run([this](flecs::iter &) {
			Sync();
			Step(_ecsWorld->DeltaTime());
		});
	_stepSystem = System(system);
	world.TrackPlayOnly(_stepSystem);
}

void PhysicsWorld::Shutdown() {
	if (_colliderRemoved.id() != 0) {
		_colliderRemoved.destruct();
		_colliderRemoved = flecs::observer();
	}
	if (_bodyRemoved.id() != 0) {
		_bodyRemoved.destruct();
		_bodyRemoved = flecs::observer();
	}
	if (_raycastRemoved.id() != 0) {
		_raycastRemoved.destruct();
		_raycastRemoved = flecs::observer();
	}
	if (_ecsWorld != nullptr && _stepSystem.IsValid()) {
		_ecsWorld->DestroyTrackedSystem(_stepSystem);
	}
	_stepSystem = {};
	_ecsWorld = nullptr;
	_bodiesQuery = {};
	_collidersQuery = {};
	_raycastsQuery = {};
	_raycasts.clear();
	_bodies.clear();
	_shapes.clear();
	_shapeEntities.clear();
	_gone.clear();
	_missingTransformWarned.clear();
	_missingBodyWarned.clear();
	_frameEvents.clear();

	if (b2World_IsValid(_worldId)) {
		b2DestroyWorld(_worldId);
	}
	_worldId = b2_nullWorldId;
	_accumulator = 0.f;
}

bool PhysicsWorld::IsInitialized() const {
	return b2World_IsValid(_worldId);
}

f32 PhysicsWorld::pixelsPerMeter() const {
	return _config.pixelsPerMeter;
}

b2Vec2 PhysicsWorld::toMeters(Vector2 pixels) const {
	const f32 ppm = pixelsPerMeter();
	return b2Vec2{pixels.x / ppm, pixels.y / ppm};
}

Vector2 PhysicsWorld::toPixels(b2Vec2 meters) const {
	const f32 ppm = pixelsPerMeter();
	return Vector2(meters.x * ppm, meters.y * ppm);
}

void PhysicsWorld::rememberGone(Entity collider, const ShapeRecord &record) {
	if (!b2Shape_IsValid(record.id)) {
		return;
	}
	GoneShape gone;
	gone.collider = collider;
	gone.body = record.owner;
	_gone[b2StoreShapeId(record.id)] = gone;
}

void PhysicsWorld::destroyBody(Entity entity, BodyRecord &record) {
	for (auto &[collider, shape] : _shapes) {
		if (shape.owner == entity) {
			rememberGone(collider, shape);
			_shapeEntities.erase(b2StoreShapeId(shape.id));
		}
	}
	if (b2Body_IsValid(record.id)) {
		b2DestroyBody(record.id);
	}
	record.id = b2_nullBodyId;
}

void PhysicsWorld::destroyShape(Entity collider, ShapeRecord &record) {
	_shapeEntities.erase(b2StoreShapeId(record.id));
	if (b2Shape_IsValid(record.id)) {
		rememberGone(collider, record);
		b2DestroyShape(record.id, true);
	}
	record.id = b2_nullShapeId;
	record.pushedBodyId = b2_nullBodyId;
}

void PhysicsWorld::createBody(Entity entity, const RigidBody2D &body, BodyRecord &record) {
	Vector2 position;
	f32 rotation = 0.f;
	Matrix::DecomposeTransform2D(ComputeWorldMatrix(entity), &position, &rotation, nullptr);

	b2BodyDef def = b2DefaultBodyDef();
	def.type = ToB2Type(body.type);
	def.position = toMeters(position);
	def.rotation = b2MakeRot(rotation * kDegToRad);
	def.linearDamping = body.linearDamping;
	def.angularDamping = body.angularDamping;
	def.gravityScale = body.gravityScale;
	def.fixedRotation = body.fixedRotation;
	def.isAwake = true;
	record.id = b2CreateBody(_worldId, &def);
	record.pushedType = body.type;
	record.pushedGravityScale = body.gravityScale;
	record.pushedLinearDamping = body.linearDamping;
	record.pushedAngularDamping = body.angularDamping;
	record.pushedFixedRotation = body.fixedRotation;
	storePose(record, position, rotation);
	applyPendingVelocities(record);
}

void PhysicsWorld::pushProperties(const RigidBody2D &body, BodyRecord &record) {
	if (!b2Body_IsValid(record.id)) {
		return;
	}
	if (record.pushedType != body.type) {
		b2Body_SetType(record.id, ToB2Type(body.type));
		record.pushedType = body.type;
	}
	if (record.pushedGravityScale != body.gravityScale) {
		b2Body_SetGravityScale(record.id, body.gravityScale);
		record.pushedGravityScale = body.gravityScale;
	}
	if (record.pushedLinearDamping != body.linearDamping) {
		b2Body_SetLinearDamping(record.id, body.linearDamping);
		record.pushedLinearDamping = body.linearDamping;
	}
	if (record.pushedAngularDamping != body.angularDamping) {
		b2Body_SetAngularDamping(record.id, body.angularDamping);
		record.pushedAngularDamping = body.angularDamping;
	}
	if (record.pushedFixedRotation != body.fixedRotation) {
		b2Body_SetFixedRotation(record.id, body.fixedRotation);
		record.pushedFixedRotation = body.fixedRotation;
	}
}

void PhysicsWorld::applyPendingVelocities(BodyRecord &record) {
	if (!b2Body_IsValid(record.id)) {
		return;
	}
	if (record.hasPendingLinearVelocity) {
		b2Body_SetLinearVelocity(record.id, toMeters(record.pendingLinearVelocityPx));
		record.hasPendingLinearVelocity = false;
	}
	if (record.hasPendingAngularVelocity) {
		b2Body_SetAngularVelocity(record.id, record.pendingAngularVelocityDeg * kDegToRad);
		record.hasPendingAngularVelocity = false;
	}
}

void PhysicsWorld::applyQueuedForces(BodyRecord &record) {
	if (!b2Body_IsValid(record.id)) {
		return;
	}
	const f32 ppm = pixelsPerMeter();
	const f32 ppm2 = ppm * ppm;
	if (record.forcePx.x != 0.f || record.forcePx.y != 0.f) {
		b2Body_ApplyForceToCenter(record.id, toMeters(record.forcePx), true);
		record.forcePx = {};
	}
	if (record.torquePx != 0.f) {
		b2Body_ApplyTorque(record.id, record.torquePx / ppm2, true);
		record.torquePx = 0.f;
	}
	if (record.impulsePx.x != 0.f || record.impulsePx.y != 0.f) {
		b2Body_ApplyLinearImpulseToCenter(record.id, toMeters(record.impulsePx), true);
		record.impulsePx = {};
	}
	if (record.angularImpulsePx != 0.f) {
		b2Body_ApplyAngularImpulse(record.id, record.angularImpulsePx / ppm2, true);
		record.angularImpulsePx = 0.f;
	}
}

void PhysicsWorld::Sync() {
	if (!IsInitialized() || !_bodiesQuery.IsValid()) {
		return;
	}
	syncBodies();
	if (_collidersQuery.IsValid()) {
		syncShapes();
	}
}

void PhysicsWorld::syncBodies(bool existingOnly) {
	std::unordered_set<Entity> live;
	_bodiesQuery.Each([&](Entity entity, RigidBody2D &body) {
		if (!entity.Has<Transform2D>()) {
			if (_missingTransformWarned.insert(entity).second) {
				Log::Warn("RigidBody2D on '{}' needs a Transform2D", entity.GetPath());
			}
			return;
		}
		_missingTransformWarned.erase(entity);
		live.insert(entity);
		const auto existing = _bodies.find(entity);
		if (existingOnly && (existing == _bodies.end() || !b2Body_IsValid(existing->second.id))) {
			return;
		}
		BodyRecord &record = _bodies[entity];
		if (!b2Body_IsValid(record.id)) {
			createBody(entity, body, record);
		} else {
			pushProperties(body, record);
		}
		applyPendingVelocities(record);
	});

	for (auto it = _bodies.begin(); it != _bodies.end();) {
		if (!it->first.IsAlive() || live.find(it->first) == live.end()) {
			destroyBody(it->first, it->second);
			_missingTransformWarned.erase(it->first);
			it = _bodies.erase(it);
		} else {
			++it;
		}
	}
}

Entity PhysicsWorld::findBodyEntity(Entity collider) const {
	for (Entity current = collider; current.IsAlive(); current = current.GetParent()) {
		if (current.Has<RigidBody2D>()) {
			return current;
		}
	}
	return {};
}

PhysicsWorld::ShapeLocal PhysicsWorld::computeShapeLocal(Entity collider, Entity owner,
	const Collider2D &colliderData) const {
	glm::mat4 bodyMatrix = ComputeWorldMatrix(owner);
	f32 bodyRot = 0.f;
	Matrix::DecomposeTransform2D(bodyMatrix, nullptr, &bodyRot, nullptr);
	// Remove world translation to avoid cancellation error in child geometry.
	bodyMatrix[3] = glm::vec4(0.f, 0.f, 0.f, 1.f);
	glm::mat4 relative(1.f);
	for (Entity current = collider; current.IsAlive() && current != owner;
		 current = current.GetParent()) {
		if (const auto *transform = current.Get<Transform2D>()) {
			relative = transform->LocalMatrix() * relative;
		}
	}

	Vector2 colPos;
	f32 colRot = 0.f;
	Vector2 colScale(1.f, 1.f);
	Matrix::DecomposeTransform2D(bodyMatrix * relative, &colPos, &colRot, &colScale);

	const Vector2 originLocal = RotatePx(colPos, -bodyRot);
	const f32 relRot = colRot - bodyRot;
	const Vector2 center = originLocal + RotatePx(colliderData.offset, relRot);
	return ShapeLocal{
		center,
		relRot + colliderData.rotation,
		Vector2(Math::Abs(colScale.x), Math::Abs(colScale.y)),
	};
}

bool PhysicsWorld::geometryChanged(const Collider2D &collider, const ShapeRecord &record,
	Entity owner, const ShapeLocal &local) const {
	if (!SameBody(record.pushedBodyId, BodyId(owner))) {
		return true;
	}
	if (record.owner != owner) {
		return true;
	}
	if (record.pushedType != collider.type) {
		return true;
	}
	if (record.pushedIsTrigger != collider.isTrigger) {
		return true;
	}
	if (record.pushedSize != collider.size) {
		return true;
	}
	if (record.pushedRadius != collider.radius) {
		return true;
	}
	if (record.pushedOffset != collider.offset) {
		return true;
	}
	if (record.pushedRotation != collider.rotation) {
		return true;
	}
	// Ignore decomposition roundoff; authored fields above compare exactly.
	if (std::abs(record.pushedCenterPx.x - local.centerPx.x) > 1e-4f ||
		std::abs(record.pushedCenterPx.y - local.centerPx.y) > 1e-4f) {
		return true;
	}
	if (std::abs(std::remainder(record.pushedLocalRotation - local.rotationDeg, 360.f)) > 1e-4f) {
		return true;
	}
	if (std::abs(record.pushedScale.x - local.scale.x) > 1e-5f ||
		std::abs(record.pushedScale.y - local.scale.y) > 1e-5f) {
		return true;
	}
	return false;
}

void PhysicsWorld::storeShape(const Collider2D &collider, ShapeRecord &record, Entity owner,
	const ShapeLocal &local) {
	record.owner = owner;
	record.pushedBodyId = BodyId(owner);
	record.pushedType = collider.type;
	record.pushedSize = collider.size;
	record.pushedRadius = collider.radius;
	record.pushedOffset = collider.offset;
	record.pushedRotation = collider.rotation;
	record.pushedIsTrigger = collider.isTrigger;
	record.pushedDensity = collider.density;
	record.pushedFriction = collider.friction;
	record.pushedRestitution = collider.restitution;
	record.pushedCollisionLayer = collider.collisionLayer;
	record.pushedCollisionMask = collider.collisionMask;
	record.pushedCenterPx = local.centerPx;
	record.pushedLocalRotation = local.rotationDeg;
	record.pushedScale = local.scale;
}

void PhysicsWorld::createShape(Entity colliderEntity, Entity owner, const Collider2D &collider,
	ShapeRecord &record, const ShapeLocal &local) {
	destroyShape(colliderEntity, record);
	const b2BodyId body = BodyId(owner);
	if (!b2Body_IsValid(body)) {
		return;
	}

	b2ShapeDef def = b2DefaultShapeDef();
	def.density = collider.density;
	def.material.friction = collider.friction;
	def.material.restitution = collider.restitution;
	def.isSensor = collider.isTrigger;
	def.filter.categoryBits = collider.collisionLayer;
	def.filter.maskBits = collider.collisionMask;
	def.enableSensorEvents = true;
	def.enableContactEvents = true;
	def.updateBodyMass = true;

	const f32 ppm = pixelsPerMeter();
	const b2Vec2 center = toMeters(local.centerPx);
	const b2Rot rot = b2MakeRot(local.rotationDeg * kDegToRad);

	switch (collider.type) {
	case ShapeType2D::Box: {
		const f32 hx = 0.5f * collider.size.x * local.scale.x / ppm;
		const f32 hy = 0.5f * collider.size.y * local.scale.y / ppm;
		if (hx <= 0.f || hy <= 0.f) {
			return;
		}
		const b2Polygon box = b2MakeOffsetBox(hx, hy, center, rot);
		record.id = b2CreatePolygonShape(body, &def, &box);
		break;
	}
	case ShapeType2D::Circle: {
		const f32 radius = collider.radius * 0.5f * (local.scale.x + local.scale.y) / ppm;
		if (radius <= 0.f) {
			return;
		}
		const b2Circle circle{center, radius};
		record.id = b2CreateCircleShape(body, &def, &circle);
		break;
	}
	case ShapeType2D::Capsule: {
		const f32 radius = collider.radius * 0.5f * (local.scale.x + local.scale.y) / ppm;
		if (radius <= 0.f) {
			return;
		}
		const f32 height = collider.size.y * local.scale.y / ppm;
		f32 halfShaft = 0.5f * (height - 2.f * radius);
		if (halfShaft < 0.f) {
			halfShaft = 0.f;
		}
		const b2Vec2 along = b2RotateVector(rot, b2Vec2{0.f, halfShaft});
		b2Capsule capsule;
		capsule.center1 = b2Vec2{center.x + along.x, center.y + along.y};
		capsule.center2 = b2Vec2{center.x - along.x, center.y - along.y};
		capsule.radius = radius;
		record.id = b2CreateCapsuleShape(body, &def, &capsule);
		break;
	}
	}

	if (b2Shape_IsValid(record.id)) {
		storeShape(collider, record, owner, local);
		_shapeEntities[b2StoreShapeId(record.id)] = colliderEntity;
	}
}

void PhysicsWorld::pushShapeMaterial(const Collider2D &collider, ShapeRecord &record) {
	if (!b2Shape_IsValid(record.id)) {
		return;
	}
	if (record.pushedCollisionLayer != collider.collisionLayer ||
		record.pushedCollisionMask != collider.collisionMask) {
		b2Filter filter = b2DefaultFilter();
		filter.categoryBits = collider.collisionLayer;
		filter.maskBits = collider.collisionMask;
		b2Shape_SetFilter(record.id, filter);
		record.pushedCollisionLayer = collider.collisionLayer;
		record.pushedCollisionMask = collider.collisionMask;
	}
	if (record.pushedDensity != collider.density) {
		b2Shape_SetDensity(record.id, collider.density, true);
		record.pushedDensity = collider.density;
	}
	if (record.pushedFriction != collider.friction) {
		b2Shape_SetFriction(record.id, collider.friction);
		record.pushedFriction = collider.friction;
	}
	if (record.pushedRestitution != collider.restitution) {
		b2Shape_SetRestitution(record.id, collider.restitution);
		record.pushedRestitution = collider.restitution;
	}
}

void PhysicsWorld::syncShapes(bool existingOnly) {
	std::unordered_set<Entity> live;
	_collidersQuery.Each([&](Entity entity, Collider2D &collider) {
		live.insert(entity);
		if (existingOnly && !_shapes.contains(entity)) {
			return;
		}
		const Entity owner = findBodyEntity(entity);
		if (!owner.IsAlive()) {
			if (_missingBodyWarned.insert(entity).second) {
				Log::Warn("Collider2D on '{}' needs a RigidBody2D on this entity or an ancestor",
					entity.GetPath());
			}
			const auto it = _shapes.find(entity);
			if (it != _shapes.end()) {
				destroyShape(it->first, it->second);
				_shapes.erase(it);
			}
			return;
		}
		_missingBodyWarned.erase(entity);

		const b2BodyId body = BodyId(owner);
		if (!b2Body_IsValid(body)) {
			const auto it = _shapes.find(entity);
			if (it != _shapes.end()) {
				destroyShape(it->first, it->second);
				_shapes.erase(it);
			}
			return;
		}

		const ShapeLocal local = computeShapeLocal(entity, owner, collider);
		ShapeRecord &record = _shapes[entity];
		if (!b2Shape_IsValid(record.id) || geometryChanged(collider, record, owner, local)) {
			createShape(entity, owner, collider, record, local);
		} else {
			pushShapeMaterial(collider, record);
		}
	});

	for (auto it = _shapes.begin(); it != _shapes.end();) {
		if (!it->first.IsAlive() || live.find(it->first) == live.end()) {
			destroyShape(it->first, it->second);
			_missingBodyWarned.erase(it->first);
			it = _shapes.erase(it);
		} else {
			++it;
		}
	}
}

void PhysicsWorld::storePose(BodyRecord &record, Vector2 worldPosPx, f32 worldRotDeg) {
	record.pushedWorldPosPx = worldPosPx;
	record.pushedWorldRotDeg = worldRotDeg;
	record.hasPose = true;
}

void PhysicsWorld::writeLocalPose(Entity entity, Transform2D &transform, Vector2 worldPosPx,
	f32 worldRotDeg) {
	Entity ancestor;
	for (Entity current = entity.GetParent(); current.IsAlive(); current = current.GetParent()) {
		if (current.Has<Transform2D>()) {
			ancestor = current;
			break;
		}
	}
	if (!ancestor.IsAlive()) {
		transform.position = worldPosPx;
		transform.rotation = worldRotDeg;
		return;
	}

	Vector2 parentPos;
	f32 parentRot = 0.f;
	Vector2 parentScale(1.f, 1.f);
	Matrix::DecomposeTransform2D(ComputeWorldMatrix(ancestor), &parentPos, &parentRot,
		&parentScale);

	Vector2 local = RotatePx(worldPosPx - parentPos, -parentRot);
	if (parentScale.x != 0.f) {
		local.x /= parentScale.x;
	}
	if (parentScale.y != 0.f) {
		local.y /= parentScale.y;
	}
	transform.position = local;
	transform.rotation = worldRotDeg - parentRot;
}

void PhysicsWorld::pushPoses() {
	for (auto &[entity, record] : _bodies) {
		Entity live = entity;
		if (!b2Body_IsValid(record.id) || !live.IsAlive()) {
			continue;
		}
		const RigidBody2D *body = live.Get<RigidBody2D>();
		if (body == nullptr || body->type == BodyType2D::Dynamic) {
			continue;
		}
		if (!live.Has<Transform2D>()) {
			continue;
		}

		Vector2 position;
		f32 rotation = 0.f;
		Matrix::DecomposeTransform2D(ComputeWorldMatrix(live), &position, &rotation, nullptr);
		if (record.hasPose && record.pushedWorldPosPx == position &&
			record.pushedWorldRotDeg == rotation) {
			continue;
		}
		b2Body_SetTransform(record.id, toMeters(position), b2MakeRot(rotation * kDegToRad));
		storePose(record, position, rotation);
	}
}

void PhysicsWorld::writePoses() {
	// Write parents first so child local poses use the new parent pose.
	std::vector<std::pair<size_t, Entity>> order;
	order.reserve(_bodies.size());
	for (const auto &[entity, record] : _bodies) {
		(void)record;
		size_t depth = 0;
		for (Entity ancestor = entity.GetParent(); ancestor.IsAlive(); ancestor = ancestor.GetParent()) {
			++depth;
		}
		order.emplace_back(depth, entity);
	}
	std::sort(order.begin(), order.end(), [](const auto &a, const auto &b) {
		return a.first < b.first;
	});
	for (const auto &[depth, entity] : order) {
		(void)depth;
		BodyRecord &record = _bodies.at(entity);
		Entity live = entity;
		if (!b2Body_IsValid(record.id) || !live.IsAlive()) {
			continue;
		}
		const RigidBody2D *body = live.Get<RigidBody2D>();
		if (body == nullptr || body->type == BodyType2D::Static) {
			continue;
		}
		Transform2D *transform = live.Get<Transform2D>();
		if (transform == nullptr) {
			continue;
		}

		const Vector2 worldPos = toPixels(b2Body_GetPosition(record.id));
		const f32 worldRot = b2Rot_GetAngle(b2Body_GetRotation(record.id)) * kRadToDeg;
		writeLocalPose(live, *transform, worldPos, worldRot);
		storePose(record, worldPos, worldRot);
	}
}

void PhysicsWorld::Step(f32 frameDelta) {
	if (!IsInitialized()) {
		return;
	}

	// Step(0) can still prepare query poses while paused.
	if (frameDelta > 0.f && _ecsWorld &&
		(!_ecsWorld->PlayOnlySystemsEnabled() || _ecsWorld->FixedTicksCancelled())) {
		return;
	}

	_frameEvents.clear();
	pushPoses();
	if (frameDelta <= 0.f) {
		return;
	}

	// Cap incoming time without losing the previous fractional remainder.
	const f64 tickDelta = _config.fixedDelta;
	const f64 cap = tickDelta * _config.maxStepsPerFrame;
	_accumulator += std::min(static_cast<f64>(frameDelta), cap);
	const f64 tolerance = tickDelta * 1e-6;

	i32 ticks = 0;
	while (ticks < _config.maxStepsPerFrame && _accumulator + tolerance >= tickDelta) {
		SOWA_PROFILE_SCOPE("Physics tick");
		auto prepare = [this] {
			Sync();
			pushPoses();
			if (_ecsWorld) {
				_ecsWorld->RefreshTransforms();
			}
		};
		auto simulate = [this] {
			// New bodies and shapes join next tick.
			if (_bodiesQuery.IsValid()) {
				syncBodies(true);
			}
			if (_collidersQuery.IsValid()) {
				syncShapes(true);
			}
			pushPoses();
			for (auto &[entity, record] : _bodies) {
				(void)entity;
				applyQueuedForces(record);
			}
			b2World_Step(_worldId, _config.fixedDelta, _config.subStepCount);
			writePoses();
			if (_ecsWorld) {
				_ecsWorld->RefreshTransforms();
			}
			sampleRaycasts();
			drainEvents();
			_gone.clear();
		};
		if (_ecsWorld) {
			if (!_ecsWorld->RunFixedTick(_config.fixedDelta, prepare, simulate)) {
				break;
			}
		} else {
			simulate();
		}
		_accumulator = std::max(0.0, _accumulator - tickDelta);
		++ticks;
	}
	SOWA_PROFILE_COUNTER("Physics ticks", ticks);
	SOWA_PROFILE_COUNTER("Physics bodies", _bodies.size());
	SOWA_PROFILE_COUNTER("Physics contacts", b2World_GetCounters(_worldId).contactCount);
}

void PhysicsWorld::drainEvents() {
	const b2ContactEvents contacts = b2World_GetContactEvents(_worldId);
	for (i32 i = 0; i < contacts.beginCount; ++i) {
		const b2ContactBeginTouchEvent &event = contacts.beginEvents[i];
		Vector2 point;
		Vector2 normal(event.manifold.normal.x, event.manifold.normal.y);
		if (event.manifold.pointCount > 0) {
			point = toPixels(event.manifold.points[0].point);
		}
		emit(CollisionKind::Begin, event.shapeIdA, event.shapeIdB, point, normal);
	}
	for (i32 i = 0; i < contacts.endCount; ++i) {
		const b2ContactEndTouchEvent &event = contacts.endEvents[i];
		emit(CollisionKind::End, event.shapeIdA, event.shapeIdB, {}, {});
	}

	const b2SensorEvents sensors = b2World_GetSensorEvents(_worldId);
	for (i32 i = 0; i < sensors.beginCount; ++i) {
		const b2SensorBeginTouchEvent &event = sensors.beginEvents[i];
		emit(CollisionKind::TriggerBegin, event.sensorShapeId, event.visitorShapeId, {}, {});
	}
	for (i32 i = 0; i < sensors.endCount; ++i) {
		const b2SensorEndTouchEvent &event = sensors.endEvents[i];
		emit(CollisionKind::TriggerEnd, event.sensorShapeId, event.visitorShapeId, {}, {});
	}
}

void PhysicsWorld::emit(CollisionKind kind, b2ShapeId shapeA, b2ShapeId shapeB, Vector2 point,
	Vector2 normal) {
	Collision2D event;
	event.kind = kind;
	event.colliderA = colliderOf(shapeA);
	event.colliderB = colliderOf(shapeB);
	event.bodyA = bodyOf(shapeA, event.colliderA);
	event.bodyB = bodyOf(shapeB, event.colliderB);
	event.point = point;
	event.normal = normal;
	_frameEvents.push_back(event);
	_events(event);
}

Entity PhysicsWorld::colliderOf(b2ShapeId id) const {
	if (b2Shape_IsValid(id)) {
		if (const auto it = _shapeEntities.find(b2StoreShapeId(id)); it != _shapeEntities.end()) {
			return it->second;
		}
	}
	const auto it = _gone.find(b2StoreShapeId(id));
	if (it != _gone.end()) {
		return it->second.collider;
	}
	return {};
}

Entity PhysicsWorld::bodyOf(b2ShapeId id, Entity collider) const {
	if (collider.IsAlive()) {
		const Entity owner = findBodyEntity(collider);
		if (owner.IsAlive()) {
			return owner;
		}
	}
	const auto it = _gone.find(b2StoreShapeId(id));
	if (it != _gone.end()) {
		return it->second.body;
	}
	return {};
}

b2BodyId PhysicsWorld::BodyId(Entity entity) const {
	const auto it = _bodies.find(entity);
	if (it == _bodies.end()) {
		return b2_nullBodyId;
	}
	return it->second.id;
}

b2ShapeId PhysicsWorld::ShapeId(Entity entity) const {
	const auto it = _shapes.find(entity);
	if (it == _shapes.end()) {
		return b2_nullShapeId;
	}
	return it->second.id;
}

bool PhysicsWorld::ensureRecord(Entity entity, BodyRecord *&record) {
	if (!IsInitialized()) {
		return false;
	}
	if (!entity.IsAlive() || !entity.Has<RigidBody2D>() || !entity.Has<Transform2D>()) {
		return false;
	}
	record = &_bodies[entity];
	return true;
}

void PhysicsWorld::ApplyForce(Entity entity, Vector2 forcePx) {
	BodyRecord *record = nullptr;
	if (!ensureRecord(entity, record)) {
		return;
	}
	record->forcePx.x += forcePx.x;
	record->forcePx.y += forcePx.y;
}

void PhysicsWorld::ApplyTorque(Entity entity, f32 torquePx) {
	BodyRecord *record = nullptr;
	if (!ensureRecord(entity, record)) {
		return;
	}
	record->torquePx += torquePx;
}

void PhysicsWorld::ApplyImpulse(Entity entity, Vector2 impulsePx) {
	BodyRecord *record = nullptr;
	if (!ensureRecord(entity, record)) {
		return;
	}
	record->impulsePx.x += impulsePx.x;
	record->impulsePx.y += impulsePx.y;
}

void PhysicsWorld::ApplyAngularImpulse(Entity entity, f32 angularImpulsePx) {
	BodyRecord *record = nullptr;
	if (!ensureRecord(entity, record)) {
		return;
	}
	record->angularImpulsePx += angularImpulsePx;
}

void PhysicsWorld::SetLinearVelocity(Entity entity, Vector2 velocityPx) {
	BodyRecord *record = nullptr;
	if (!ensureRecord(entity, record)) {
		return;
	}
	if (b2Body_IsValid(record->id)) {
		b2Body_SetLinearVelocity(record->id, toMeters(velocityPx));
		record->hasPendingLinearVelocity = false;
		return;
	}
	record->pendingLinearVelocityPx = velocityPx;
	record->hasPendingLinearVelocity = true;
}

Vector2 PhysicsWorld::GetLinearVelocity(Entity entity) const {
	const auto it = _bodies.find(entity);
	if (it == _bodies.end()) {
		return {};
	}
	if (b2Body_IsValid(it->second.id)) {
		return toPixels(b2Body_GetLinearVelocity(it->second.id));
	}
	if (it->second.hasPendingLinearVelocity) {
		return it->second.pendingLinearVelocityPx;
	}
	return {};
}

void PhysicsWorld::SetAngularVelocity(Entity entity, f32 degreesPerSecond) {
	BodyRecord *record = nullptr;
	if (!ensureRecord(entity, record)) {
		return;
	}
	if (b2Body_IsValid(record->id)) {
		b2Body_SetAngularVelocity(record->id, degreesPerSecond * kDegToRad);
		record->hasPendingAngularVelocity = false;
		return;
	}
	record->pendingAngularVelocityDeg = degreesPerSecond;
	record->hasPendingAngularVelocity = true;
}

f32 PhysicsWorld::GetAngularVelocity(Entity entity) const {
	const auto it = _bodies.find(entity);
	if (it == _bodies.end()) {
		return 0.f;
	}
	if (b2Body_IsValid(it->second.id)) {
		return b2Body_GetAngularVelocity(it->second.id) * kRadToDeg;
	}
	if (it->second.hasPendingAngularVelocity) {
		return it->second.pendingAngularVelocityDeg;
	}
	return 0.f;
}

void PhysicsWorld::DebugDraw(const std::function<void(Vector2, Vector2, Color)> &drawLine) const {
	if (!IsInitialized() || !drawLine) {
		return;
	}

	DebugDrawState state;
	state.pixelsPerMeter = _config.pixelsPerMeter;
	state.drawLine = &drawLine;

	b2DebugDraw debug = b2DefaultDebugDraw();
	debug.DrawPolygonFcn = DrawPolygon;
	debug.DrawSolidPolygonFcn = DrawSolidPolygon;
	debug.DrawCircleFcn = DrawCircle;
	debug.DrawSolidCircleFcn = DrawSolidCircle;
	debug.DrawSolidCapsuleFcn = DrawSolidCapsule;
	debug.DrawSegmentFcn = DrawSegment;
	debug.DrawTransformFcn = DrawTransform;
	debug.DrawPointFcn = DrawPoint;
	debug.DrawStringFcn = DrawString;
	debug.drawShapes = true;
	debug.drawJoints = true;
	debug.context = &state;
	b2World_Draw(_worldId, &debug);
	if (_ecsWorld != nullptr) {
		_ecsWorld->Each<Raycast2D>([&](Entity ray, Raycast2D &) {
			Vector2 from, to;
			if (!raycastEndpoints(ray, from, to)) {
				return;
			}
			drawLine(from, to, Color::RGB(100, 210, 255));
			if (auto it = _raycasts.find(ray); it != _raycasts.end() && it->second.hit) {
				const auto &hit = *it->second.hit;
				PhysicsQueryOptions2D options;
				options.includeSensors = true;
				if (b2StoreShapeId(ShapeId(hit.collider)) == it->second.hitShapeId &&
					queryTarget(ShapeId(hit.collider), options)) {
					drawLine(hit.point - Vector2(3.f, 0.f), hit.point + Vector2(3.f, 0.f),
						Color::RGB(255, 220, 80));
					drawLine(hit.point, hit.point + hit.normal * 20.f, Color::RGB(255, 220, 80));
				}
			}
		});
	}
}

} // namespace sowa
