#pragma once

#include <functional>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <box2d/box2d.h>
#include <eventpp/callbacklist.h>

#include "core/types.hpp"
#include "ecs/entity.hpp"
#include "ecs/query.hpp"
#include "ecs/system.hpp"
#include "math/color.hpp"
#include "math/vector2.hpp"
#include "physics/collider2d.hpp"
#include "physics/collision2d.hpp"
#include "physics/raycast2d.hpp"
#include "physics/rigid_body2d.hpp"

namespace sowa {

class World;
struct Transform2D;

// Owns Box2D handles and fixed-step simulation. Runs after gameplay in OnValidate,
// before world transform calculation. Converts pixels/degrees to metres/radians.
// Must be destroyed before World.
struct PhysicsWorldConfig {
	// Metres per second squared. Engine space is Y-up, so the default pulls
	// toward -Y. Box2D has no up vector of its own.
	Vector2 gravity{0.f, -9.8f};
	// Length of one b2World_Step. Frame delta is accumulated against this
	// rather than passed through: Box2D is not stable on a variable step.
	f32 fixedDelta = 1.f / 60.f;
	// Soft-step sub-steps per tick. Box2D's recommended starting value.
	i32 subStepCount = 4;
	// Conversion between Transform2D pixels and Box2D metres.
	f32 pixelsPerMeter = 100.f;
	// Cap on b2World_Step calls per Step(). A hitch would otherwise try to
	// catch up every skipped tick and never recover (spiral of death).
	i32 maxStepsPerFrame = 8;
};

class PhysicsWorld {
  public:
	PhysicsWorld() = default;
	~PhysicsWorld();

	PhysicsWorld(const PhysicsWorld &) = delete;
	PhysicsWorld &operator=(const PhysicsWorld &) = delete;
	PhysicsWorld(PhysicsWorld &&) = delete;
	PhysicsWorld &operator=(PhysicsWorld &&) = delete;

	// Creates the Box2D world. Safe to call again: the previous world is
	// destroyed first. No flecs system - tests that call Step() by hand
	// use this overload so they do not need a World.
	void Init(const PhysicsWorldConfig &config = {});

	// Initializes physics, registers components and queries, and adds a play-only
	// OnValidate system that syncs and steps from Progress. Step writes poses to Transform2D.
	void Init(World &world, const PhysicsWorldConfig &config = {});

	void Shutdown();
	// Clears solver/session state without unregistering components or systems.
	void ResetSimulation();
	bool IsInitialized() const;

	// Read-only queries of the synchronized solver snapshot. Native rays use
	// origin + translation; Lua uses from/to endpoints. No implicit Sync/Step.
	PhysicsClosestResult2D RaycastClosest(Vector2 origin, Vector2 translation,
		const PhysicsQueryOptions2D &options = {}) const;
	PhysicsHitsResult2D RaycastAll(Vector2 origin, Vector2 translation,
		const PhysicsQueryOptions2D &options = {}) const;
	PhysicsClosestResult2D ShapeCastClosest(const PhysicsQueryShape2D &shape, Vector2 translation,
		const PhysicsQueryOptions2D &options = {}) const;
	PhysicsHitsResult2D ShapeCastAll(const PhysicsQueryShape2D &shape, Vector2 translation,
		const PhysicsQueryOptions2D &options = {}) const;
	PhysicsOverlapsResult2D OverlapShape(const PhysicsQueryShape2D &shape,
		const PhysicsQueryOptions2D &options = {}) const;
	PhysicsClosestResult2D ForceRaycastUpdate(Entity ray);
	std::optional<PhysicsHit2D> RaycastHit(Entity ray);
	bool AddRaycastException(Entity ray, Entity excluded, bool body);
	void RemoveRaycastException(Entity ray, Entity excluded, bool body);
	void ClearRaycastExceptions(Entity ray);

	// Syncs bodies, shapes, and authored fields before Step. Skips disabled entities.
	// Bodies need Transform2D; colliders need a body on self or an ancestor.
	void Sync();

	// Accumulates fixed steps and preserves the remainder. Forces apply each tick;
	// impulses apply once. Writes dynamic/kinematic poses to local Transform2D.
	// Non-positive delta still syncs static/kinematic poses. No-op before initialization.
	void Step(f32 frameDelta);

	const PhysicsWorldConfig &Config() const {
		return _config;
	}

	f32 Accumulator() const {
		return _accumulator;
	}

	// Native world handle. Invalid before initialization.
	b2WorldId WorldId() const {
		return _worldId;
	}

	// Body for this entity, or b2_nullBodyId if Sync has not created one.
	b2BodyId BodyId(Entity entity) const;

	// Shape for this collider entity, or b2_nullShapeId if Sync has not
	// created one. Keyed on the Collider2D entity, not the body.
	b2ShapeId ShapeId(Entity entity) const;

	// Pixel-space force, reapplied at the start of every physics tick in
	// the current Step, then dropped. No-op without RigidBody2D+Transform2D.
	void ApplyForce(Entity entity, Vector2 forcePx);
	// Pixel-space torque (force * lever in pixels). Same lifetime as force.
	void ApplyTorque(Entity entity, f32 torquePx);
	// Pixel-space impulse, applied once on the next physics tick.
	void ApplyImpulse(Entity entity, Vector2 impulsePx);
	void ApplyAngularImpulse(Entity entity, f32 angularImpulsePx);

	void SetLinearVelocity(Entity entity, Vector2 velocityPx);
	Vector2 GetLinearVelocity(Entity entity) const;
	void SetAngularVelocity(Entity entity, f32 degreesPerSecond);
	f32 GetAngularVelocity(Entity entity) const;

	using EventCallbackList = eventpp::CallbackList<void(const Collision2D &)>;

	EventCallbackList &Events() {
		return _events;
	}

	// Events drained from every b2World_Step in the last Step() call.
	// Cleared at the start of Step. Empty if no tick ran.
	const std::vector<Collision2D> &FrameEvents() const {
		return _frameEvents;
	}

	// Reports debug outline segments in pixel space through drawLine. No-op before initialization.
	void DebugDraw(const std::function<void(Vector2, Vector2, Color)> &drawLine) const;

	void SetDebugDrawEnabled(bool enabled) {
		_debugDrawEnabled = enabled;
	}

	bool DebugDrawEnabled() const {
		return _debugDrawEnabled;
	}

  private:
	struct BodyRecord {
		b2BodyId id = b2_nullBodyId;
		Vector2 forcePx;
		f32 torquePx = 0.f;
		Vector2 impulsePx;
		f32 angularImpulsePx = 0.f;
		bool hasPendingLinearVelocity = false;
		Vector2 pendingLinearVelocityPx;
		bool hasPendingAngularVelocity = false;
		f32 pendingAngularVelocityDeg = 0.f;
		BodyType2D pushedType = BodyType2D::Dynamic;
		f32 pushedGravityScale = 1.f;
		f32 pushedLinearDamping = 0.f;
		f32 pushedAngularDamping = 0.f;
		bool pushedFixedRotation = false;
		bool hasPose = false;
		Vector2 pushedWorldPosPx;
		f32 pushedWorldRotDeg = 0.f;
	};

	struct ShapeRecord {
		b2ShapeId id = b2_nullShapeId;
		Entity owner;
		b2BodyId pushedBodyId = b2_nullBodyId;
		ShapeType2D pushedType = ShapeType2D::Box;
		Vector2 pushedSize{100.f, 100.f};
		f32 pushedRadius = 50.f;
		Vector2 pushedOffset;
		f32 pushedRotation = 0.f;
		bool pushedIsTrigger = false;
		f32 pushedDensity = 1.f;
		f32 pushedFriction = 0.6f;
		f32 pushedRestitution = 0.f;
		u32 pushedCollisionLayer = 1u;
		u32 pushedCollisionMask = AllPhysicsLayers;
		Vector2 pushedCenterPx;
		f32 pushedLocalRotation = 0.f;
		Vector2 pushedScale{1.f, 1.f};
	};

	struct GoneShape {
		Entity collider;
		Entity body;
	};

	struct ShapeLocal {
		Vector2 centerPx;
		f32 rotationDeg = 0.f;
		Vector2 scale{1.f, 1.f};
	};

	struct RaycastRecord {
		std::optional<PhysicsHit2D> hit;
		std::vector<Entity> excludeColliders;
		std::vector<Entity> excludeBodies;
		u64 hitShapeId = 0;
		bool enabled = true;
		bool warned = false;
	};
	struct QueryContext;
	std::optional<PhysicsOverlap2D> queryTarget(b2ShapeId id,
		const PhysicsQueryOptions2D &options) const;
	std::string validateOptions(const PhysicsQueryOptions2D &options) const;
	std::string queryProxy(const PhysicsQueryShape2D &shape, b2ShapeProxy &proxy) const;
	PhysicsHitsResult2D cast(Vector2 origin, Vector2 translation, const b2ShapeProxy *proxy,
		const PhysicsQueryOptions2D &options) const;
	static bool collectOverlap(b2ShapeId id, void *context);
	static f32 collectCast(b2ShapeId id, b2Vec2 point, b2Vec2 normal, f32 fraction, void *context);
	void sampleRaycasts();
	void pruneRaycasts();
	bool raycastLive(Entity ray) const;
	bool refreshRaycastRecord(Entity ray, RaycastRecord &record);
	bool raycastEndpoints(Entity ray, Vector2 &from, Vector2 &to) const;

	void createWorld(const PhysicsWorldConfig &config);
	void bindWorld(World &world);
	void registerStepSystem(World &world);
	void syncBodies();
	void syncShapes();
	bool ensureRecord(Entity entity, BodyRecord *&record);
	void createBody(Entity entity, const RigidBody2D &body, BodyRecord &record);
	void pushProperties(const RigidBody2D &body, BodyRecord &record);
	void applyPendingVelocities(BodyRecord &record);
	void applyQueuedForces(BodyRecord &record);
	void destroyBody(Entity entity, BodyRecord &record);
	void rememberGone(Entity collider, const ShapeRecord &record);
	Entity findBodyEntity(Entity collider) const;
	ShapeLocal computeShapeLocal(Entity collider, Entity owner,
		const Collider2D &colliderData) const;
	bool geometryChanged(const Collider2D &collider, const ShapeRecord &record, Entity owner,
		const ShapeLocal &local) const;
	void createShape(Entity colliderEntity, Entity owner, const Collider2D &collider,
		ShapeRecord &record, const ShapeLocal &local);
	void pushShapeMaterial(const Collider2D &collider, ShapeRecord &record);
	void storeShape(const Collider2D &collider, ShapeRecord &record, Entity owner,
		const ShapeLocal &local);
	void destroyShape(Entity collider, ShapeRecord &record);
	void drainEvents();
	void emit(CollisionKind kind, b2ShapeId shapeA, b2ShapeId shapeB, Vector2 point,
		Vector2 normal);
	Entity colliderOf(b2ShapeId id) const;
	Entity bodyOf(b2ShapeId id, Entity collider) const;
	void pushPoses();
	void writePoses();
	void writeLocalPose(Entity entity, Transform2D &transform, Vector2 worldPosPx, f32 worldRotDeg);
	void storePose(BodyRecord &record, Vector2 worldPosPx, f32 worldRotDeg);
	f32 pixelsPerMeter() const;
	b2Vec2 toMeters(Vector2 pixels) const;
	Vector2 toPixels(b2Vec2 meters) const;

	PhysicsWorldConfig _config;
	b2WorldId _worldId = b2_nullWorldId;
	World *_ecsWorld = nullptr;
	System _stepSystem;
	Query<RigidBody2D> _bodiesQuery;
	Query<Collider2D> _collidersQuery;
	Query<Raycast2D> _raycastsQuery;
	flecs::observer _raycastRemoved;
	flecs::observer _colliderRemoved;
	flecs::observer _bodyRemoved;
	std::unordered_map<Entity, BodyRecord> _bodies;
	std::unordered_map<Entity, ShapeRecord> _shapes;
	// Full entity generations survive 32-bit targets; never encode IDs in pointers.
	std::unordered_map<u64, Entity> _shapeEntities;
	std::unordered_map<Entity, RaycastRecord> _raycasts;
	std::unordered_map<u64, GoneShape> _gone;
	std::unordered_set<Entity> _missingTransformWarned;
	std::unordered_set<Entity> _missingBodyWarned;
	EventCallbackList _events;
	std::vector<Collision2D> _frameEvents;
	bool _debugDrawEnabled = false;
	f32 _accumulator = 0.f;
};

} // namespace sowa
