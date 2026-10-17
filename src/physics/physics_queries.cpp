#include "physics/physics_world.hpp"

#include <algorithm>
#include <cmath>
#include <glm/gtc/matrix_inverse.hpp>

#include "core/log.hpp"
#include "ecs/components/transform2d.hpp"
#include "ecs/world.hpp"
#include "math/math.hpp"
#include "math/matrix.hpp"

namespace sowa {
namespace {
bool Finite(Vector2 v) {
	return std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.LengthSquared());
}

bool Contains(const std::vector<Entity> &list, Entity entity) {
	return std::find(list.begin(), list.end(), entity) != list.end();
}

b2QueryFilter Filter(const PhysicsQueryOptions2D &options) {
	return {options.collisionLayer, options.collisionMask};
}

PhysicsClosestResult2D Closest(PhysicsHitsResult2D result) {
	PhysicsClosestResult2D closest;
	closest.error = std::move(result.error);
	if (!result.value.empty()) {
		closest.value = result.value.front();
	}
	return closest;
}
} // namespace

struct PhysicsWorld::QueryContext {
	const PhysicsWorld &world;
	const PhysicsQueryOptions2D &options;
	std::vector<PhysicsOverlap2D> overlaps;
	std::vector<PhysicsHit2D> hits;
	std::unordered_set<u64> initial;
	f32 length = 0.f;
};

std::string PhysicsWorld::validateOptions(const PhysicsQueryOptions2D &options) const {
	// Gameplay entity lookups can use a Flecs stage; compare its owning world.
	for (const auto *list : {&options.excludeColliders, &options.excludeBodies}) {
		for (Entity entity : *list) {
			if (entity.Raw().id() != 0 && _ecsWorld != nullptr &&
				ecs_get_world(entity.Raw().world().c_ptr()) != _ecsWorld->Flecs().c_ptr()) {
				return "physics query exclusion belongs to another world";
			}
		}
	}
	return {};
}

std::optional<PhysicsOverlap2D> PhysicsWorld::queryTarget(b2ShapeId id,
	const PhysicsQueryOptions2D &options) const {
	if (_ecsWorld == nullptr || !b2Shape_IsValid(id)) {
		return {};
	}
	Entity collider = colliderOf(id);
	auto it = _shapes.find(collider);
	if (it == _shapes.end() || b2StoreShapeId(it->second.id) != b2StoreShapeId(id) ||
		!collider.IsEnabled() || !collider.Has<Collider2D>()) {
		return {};
	}
	Entity body = it->second.owner;
	if (!body.IsEnabled() || !body.Has<RigidBody2D>() || !body.Has<Transform2D>() ||
		findBodyEntity(collider) != body || Contains(options.excludeColliders, collider) ||
		Contains(options.excludeBodies, body)) {
		return {};
	}
	for (Entity current = collider.GetParent(); current.IsAlive(); current = current.GetParent()) {
		if (!current.IsEnabled()) {
			return {};
		}
	}
	const bool sensor = b2Shape_IsSensor(id);
	if (sensor && !options.includeSensors) {
		return {};
	}
	return PhysicsOverlap2D{collider, body, sensor};
}

std::string PhysicsWorld::queryProxy(const PhysicsQueryShape2D &shape, b2ShapeProxy &proxy) const {
	if (!Finite(shape.position) || !std::isfinite(shape.rotation)) {
		return "physics query position and rotation must be finite";
	}
	const f32 ppm = pixelsPerMeter();
	const f32 angle = shape.rotation * (Math::Pi / 180.f);
	if (!std::isfinite(angle)) {
		return "physics query rotation is out of range";
	}
	const b2Rot rotation = b2MakeRot(angle);
	b2Vec2 points[4];
	int count = 0;
	f32 radius = 0.f;
	switch (shape.type) {
	case ShapeType2D::Box:
		if (!Finite(shape.size) || shape.size.x <= 0.f || shape.size.y <= 0.f) {
			return "physics query box size must be positive and finite";
		}
		points[0] = {-shape.size.x / (2.f * ppm), -shape.size.y / (2.f * ppm)};
		points[1] = {shape.size.x / (2.f * ppm), -shape.size.y / (2.f * ppm)};
		points[2] = {shape.size.x / (2.f * ppm), shape.size.y / (2.f * ppm)};
		points[3] = {-shape.size.x / (2.f * ppm), shape.size.y / (2.f * ppm)};
		count = 4;
		break;
	case ShapeType2D::Circle:
	case ShapeType2D::Capsule:
		if (!std::isfinite(shape.radius) || shape.radius <= 0.f ||
			!std::isfinite(shape.radius * shape.radius)) {
			return "physics query radius must be positive and finite";
		}
		radius = shape.radius / ppm;
		points[0] = {};
		count = 1;
		if (shape.type == ShapeType2D::Capsule) {
			if (!std::isfinite(shape.size.y) || shape.size.y < 2.f * shape.radius ||
				!std::isfinite(shape.size.y * shape.size.y)) {
				return "physics query capsule height must be finite and at least its diameter";
			}
			const f32 half = (shape.size.y * 0.5f - shape.radius) / ppm;
			points[0] = {0.f, -half};
			points[1] = {0.f, half};
			count = 2;
		}
		break;
	default:
		return "unsupported physics query shape";
	}
	proxy = b2MakeOffsetProxy(points, count, radius, toMeters(shape.position), rotation);
	for (int i = 0; i < proxy.count; ++i) {
		if (!Finite(Vector2(proxy.points[i].x, proxy.points[i].y))) {
			return "physics query geometry is out of range";
		}
	}
	return {};
}

bool PhysicsWorld::collectOverlap(b2ShapeId id, void *context) {
	auto &query = *static_cast<QueryContext *>(context);
	if (auto target = query.world.queryTarget(id, query.options)) {
		query.overlaps.push_back(*target);
		query.initial.insert(b2StoreShapeId(id));
	}
	return true;
}

f32 PhysicsWorld::collectCast(b2ShapeId id, b2Vec2 point, b2Vec2 normal, f32 fraction,
	void *context) {
	auto &query = *static_cast<QueryContext *>(context);
	if (query.initial.contains(b2StoreShapeId(id))) {
		return -1.f;
	}
	if (auto target = query.world.queryTarget(id, query.options)) {
		PhysicsHit2D hit;
		static_cast<PhysicsOverlap2D &>(hit) = *target;
		hit.point = query.world.toPixels(point);
		hit.normal = {normal.x, normal.y};
		hit.fraction = fraction;
		hit.distance = fraction * query.length;
		query.hits.push_back(hit);
	}
	// Collect all before ordering, including equal-distance closest candidates.
	return 1.f;
}

PhysicsHitsResult2D PhysicsWorld::cast(Vector2 origin, Vector2 translation,
	const b2ShapeProxy *proxy, const PhysicsQueryOptions2D &options) const {
	PhysicsHitsResult2D result;
	if (!Finite(origin) || !Finite(translation) || !Finite(origin + translation)) {
		result.error = "physics cast endpoints and translation must be finite and in range";
		return result;
	}
	result.error = validateOptions(options);
	if (!result || !IsInitialized() || translation.LengthSquared() == 0.f) {
		return result;
	}
	QueryContext query{*this, options};
	query.length = translation.Length();
	b2ShapeProxy start{};
	if (proxy) {
		start = *proxy;
	} else {
		start.count = 1;
		start.points[0] = toMeters(origin);
	}
	b2World_OverlapShape(_worldId, &start, Filter(options), collectOverlap, &query);
	if (proxy) {
		b2World_CastShape(_worldId, proxy, toMeters(translation), Filter(options), collectCast,
			&query);
	} else {
		b2World_CastRay(_worldId, toMeters(origin), toMeters(translation), Filter(options),
			collectCast, &query);
	}
	std::sort(query.hits.begin(), query.hits.end(), [](const auto &a, const auto &b) {
		return a.fraction != b.fraction ? a.fraction < b.fraction
										: a.collider.Raw().id() < b.collider.Raw().id();
	});
	result.value = std::move(query.hits);
	return result;
}

PhysicsHitsResult2D PhysicsWorld::RaycastAll(Vector2 origin, Vector2 translation,
	const PhysicsQueryOptions2D &options) const {
	return cast(origin, translation, nullptr, options);
}

PhysicsClosestResult2D PhysicsWorld::RaycastClosest(Vector2 origin, Vector2 translation,
	const PhysicsQueryOptions2D &options) const {
	return Closest(RaycastAll(origin, translation, options));
}

PhysicsHitsResult2D PhysicsWorld::ShapeCastAll(const PhysicsQueryShape2D &shape,
	Vector2 translation, const PhysicsQueryOptions2D &options) const {
	b2ShapeProxy proxy{};
	if (auto error = queryProxy(shape, proxy); !error.empty()) {
		return {{}, std::move(error)};
	}
	return cast(shape.position, translation, &proxy, options);
}

PhysicsClosestResult2D PhysicsWorld::ShapeCastClosest(const PhysicsQueryShape2D &shape,
	Vector2 translation, const PhysicsQueryOptions2D &options) const {
	return Closest(ShapeCastAll(shape, translation, options));
}

PhysicsOverlapsResult2D PhysicsWorld::OverlapShape(const PhysicsQueryShape2D &shape,
	const PhysicsQueryOptions2D &options) const {
	PhysicsOverlapsResult2D result;
	b2ShapeProxy proxy{};
	result.error = queryProxy(shape, proxy);
	if (!result) {
		return result;
	}
	result.error = validateOptions(options);
	if (!result || !IsInitialized()) {
		return result;
	}
	QueryContext query{*this, options};
	b2World_OverlapShape(_worldId, &proxy, Filter(options), collectOverlap, &query);
	std::sort(query.overlaps.begin(), query.overlaps.end(),
		[](const auto &a, const auto &b) { return a.collider.Raw().id() < b.collider.Raw().id(); });
	result.value = std::move(query.overlaps);
	return result;
}

bool PhysicsWorld::raycastLive(Entity ray) const {
	if (_ecsWorld == nullptr || ray.Raw().id() == 0 ||
		ecs_get_world(ray.Raw().world().c_ptr()) != _ecsWorld->Flecs().c_ptr() ||
		!ray.IsEnabled() || !ray.Has<Raycast2D>() || !ray.Has<Transform2D>()) {
		return false;
	}
	for (Entity current = ray.GetParent(); current.IsAlive(); current = current.GetParent()) {
		if (!current.IsEnabled()) {
			return false;
		}
	}
	return true;
}

bool PhysicsWorld::raycastEndpoints(Entity ray, Vector2 &from, Vector2 &to) const {
	if (!raycastLive(ray)) {
		return false;
	}
	const Raycast2D *data = ray.Get<Raycast2D>();
	glm::mat4 matrix = ComputeWorldMatrix(ray);
	Entity owner = findBodyEntity(ray);
	if (const b2BodyId id = BodyId(owner); b2Body_IsValid(id)) {
		const glm::mat4 authored = ComputeWorldMatrix(owner);
		Vector2 pos;
		f32 rot;
		Matrix::DecomposeTransform2D(authored, &pos, &rot, nullptr);
		if (std::abs(glm::determinant(authored)) > 1.e-12f) {
			const auto solver = Matrix::CalculateTransform2D(toPixels(b2Body_GetPosition(id)),
				b2Rot_GetAngle(b2Body_GetRotation(id)) * (180.f / Math::Pi), Vector2(1.f));
			const auto authoredPose = Matrix::CalculateTransform2D(pos, rot, Vector2(1.f));
			matrix = solver * glm::inverse(authoredPose) * matrix;
		}
	}
	const auto a = matrix * glm::vec4(0.f, 0.f, 0.f, 1.f);
	const auto b = matrix * glm::vec4(data->targetPosition.x, data->targetPosition.y, 0.f, 1.f);
	from = {a.x, a.y};
	to = {b.x, b.y};
	return true;
}

PhysicsClosestResult2D PhysicsWorld::ForceRaycastUpdate(Entity ray) {
	Vector2 from, to;
	if (!raycastEndpoints(ray, from, to)) {
		_raycasts.erase(ray);
		return {};
	}
	const auto &data = *ray.Get<Raycast2D>();
	auto &record = _raycasts[ray];
	PhysicsQueryOptions2D options;
	options.collisionLayer = data.collisionLayer;
	options.collisionMask = data.collisionMask;
	options.includeSensors = data.includeSensors;
	options.excludeBodies = record.excludeBodies;
	options.excludeColliders = record.excludeColliders;
	if (data.excludeOwner) {
		if (Entity owner = findBodyEntity(ray); owner.IsAlive()) {
			options.excludeBodies.push_back(owner);
		}
	}
	auto result = RaycastClosest(from, to - from, options);
	record.hit = result.value;
	record.hitShapeId = result.value ? b2StoreShapeId(ShapeId(result.value->collider)) : 0;
	record.enabled = data.enabled;
	if (result) {
		record.warned = false;
	}
	return result;
}

bool PhysicsWorld::refreshRaycastRecord(Entity ray, RaycastRecord &record) {
	if (!raycastLive(ray)) {
		return false;
	}
	const bool enabled = ray.Get<Raycast2D>()->enabled;
	if (record.enabled && !enabled) {
		record.hit.reset();
	}
	record.enabled = enabled;
	for (auto *list : {&record.excludeBodies, &record.excludeColliders}) {
		std::erase_if(*list, [](Entity entity) { return !entity.IsAlive(); });
	}
	return true;
}

void PhysicsWorld::pruneRaycasts() {
	for (auto it = _raycasts.begin(); it != _raycasts.end();) {
		if (!refreshRaycastRecord(it->first, it->second)) {
			it = _raycasts.erase(it);
		} else {
			++it;
		}
	}
}

void PhysicsWorld::sampleRaycasts() {
	pruneRaycasts();
	_raycastsQuery.Each([&](Entity ray, Raycast2D &data) {
		if (!data.enabled) {
			return;
		}
		auto result = ForceRaycastUpdate(ray);
		if (!result && !_raycasts[ray].warned) {
			Log::Warn("Raycast2D on '{}': {}", ray.GetPath(), result.error);
			_raycasts[ray].warned = true;
		}
	});
}

std::optional<PhysicsHit2D> PhysicsWorld::RaycastHit(Entity ray) {
	auto it = _raycasts.find(ray);
	if (it != _raycasts.end() && !refreshRaycastRecord(ray, it->second)) {
		_raycasts.erase(it);
		return {};
	}
	if (it == _raycasts.end() || !it->second.hit) {
		return {};
	}
	PhysicsQueryOptions2D options;
	options.includeSensors = true;
	const auto hit = *it->second.hit;
	if (b2StoreShapeId(ShapeId(hit.collider)) != it->second.hitShapeId ||
		!queryTarget(ShapeId(hit.collider), options) || findBodyEntity(hit.collider) != hit.body) {
		it->second.hit.reset();
		return {};
	}
	return hit;
}

bool PhysicsWorld::AddRaycastException(Entity ray, Entity excluded, bool body) {
	Vector2 from, to;
	if (!raycastEndpoints(ray, from, to) || !excluded.IsAlive() ||
		ecs_get_world(excluded.Raw().world().c_ptr()) != _ecsWorld->Flecs().c_ptr()) {
		return false;
	}
	auto &record = _raycasts[ray];
	auto &list = body ? record.excludeBodies : record.excludeColliders;
	if (!Contains(list, excluded)) {
		list.push_back(excluded);
	}
	return true;
}

void PhysicsWorld::RemoveRaycastException(Entity ray, Entity excluded, bool body) {
	if (auto it = _raycasts.find(ray); it != _raycasts.end()) {
		std::erase(body ? it->second.excludeBodies : it->second.excludeColliders, excluded);
	}
}

void PhysicsWorld::ClearRaycastExceptions(Entity ray) {
	if (auto it = _raycasts.find(ray); it != _raycasts.end()) {
		it->second.excludeBodies.clear();
		it->second.excludeColliders.clear();
	}
}

void PhysicsWorld::ResetSimulation() {
	if (!IsInitialized()) {
		return;
	}
	b2DestroyWorld(_worldId);
	b2WorldDef def = b2DefaultWorldDef();
	def.gravity = {_config.gravity.x, _config.gravity.y};
	_worldId = b2CreateWorld(&def);
	_bodies.clear();
	_shapes.clear();
	_shapeEntities.clear();
	_raycasts.clear();
	_gone.clear();
	_frameEvents.clear();
	_missingTransformWarned.clear();
	_missingBodyWarned.clear();
	_accumulator = 0.f;
}
} // namespace sowa
