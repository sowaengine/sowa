#include "ecs/world.hpp"
#include "animation/animation.hpp"
#include "ecs/components/animated_sprite.hpp"
#include "ecs/components/nine_patch_rect.hpp"
#include "ecs/components/parallax2d.hpp"
#include "particles/particles2d.hpp"
#include "ui/components.hpp"

#include <algorithm>
#include <cstring>
#include <limits>
#include <memory>
#include <string>
#include <vector>

#include <flecs/private/api_support.h>
#include <glm/glm.hpp>

#include "core/log.hpp"
#include "core/profiler.hpp"
#include "core/reflection/field_ref.hpp"
#include "ecs/components/camera2d.hpp"
#include "ecs/components/render_order.hpp"
#include "ecs/components/scene_root.hpp"
#include "ecs/components/text2d.hpp"
#include "ecs/components/tilemap.hpp"
#include "ecs/components/transform2d.hpp"
#include "ecs/entity.hpp"
#include "ecs/field_registry.hpp"
#include "ecs/script_tags.hpp"
#include "ecs/visibility.hpp"
#include "math/color.hpp"
#include "math/matrix.hpp"
#include "math/vector2.hpp"
#include "math/vector3.hpp"
#include "resource/resource_id.hpp"

namespace sowa {

struct World::ImmediateMutate {
	ecs_suspend_readonly_state_t state{};
	bool active = false;
};

#ifdef FLECS_PERF_TRACE
namespace {

// Forward flecs system perf-trace scopes to the profiler. Skip internal flecs scopes
// symmetrically on push and pop so only systems occupy the trace stack.
std::vector<Profiler::ScopeToken> &systemTraceStack() {
	static std::vector<Profiler::ScopeToken> stack;
	return stack;
}

bool isFlecsInternalTrace(const char *name) {
	return name == nullptr || std::strncmp(name, "flecs.", 6) == 0;
}

// The traced name is the system's flecs path, which escapes the "." path
// separator: "Lua.Player._update" arrives as "Lua\.Player\._update".
ProfileName internSystemName(Profiler &profiler, const char *name) {
	if (std::strchr(name, '\\') == nullptr) {
		return profiler.Intern(name);
	}
	std::string unescaped;
	for (const char *c = name; *c != '\0'; ++c) {
		if (*c == '\\' && c[1] != '\0') {
			++c;
		}
		unescaped.push_back(*c);
	}
	return profiler.Intern(unescaped);
}

void pushSystemTrace(const char *, size_t, const char *name) {
	if (isFlecsInternalTrace(name)) {
		return;
	}
	Profiler &profiler = Profiler::Main();
	systemTraceStack().push_back(profiler.IsRecording()
									 ? profiler.BeginScope(internSystemName(profiler, name))
									 : Profiler::ScopeToken{});
}

void popSystemTrace(const char *, size_t, const char *name) {
	auto &stack = systemTraceStack();
	if (isFlecsInternalTrace(name) || stack.empty()) {
		return;
	}
	const Profiler::ScopeToken token = stack.back();
	stack.pop_back();
	Profiler::Main().EndScope(token);
}

} // namespace
#endif

World::World() : _world(), _immediate(std::make_unique<ImmediateMutate>()) {
#ifdef FLECS_PERF_TRACE
	// ecs_os_api is process-global; flecs initialized it when _world was made.
	ecs_os_api.perf_trace_push_ = pushSystemTrace;
	ecs_os_api.perf_trace_pop_ = popSystemTrace;
#endif

	// Entity::QueueDestroy retrieves its World through the flecs context.
	_world.set_ctx(this);

	// Registered up front so ComponentType::Fields() only ever looks the
	// component up, never triggers an implicit registration mid-iteration.
	_world.component<FieldTable>("FieldTable");
	_world.component<DataDescribed>("DataDescribed");
	_world.component<ScriptLive>("ScriptLive");

	registerBuiltins();
}

void World::registerBuiltins() {
	RegisterUIComponents(*this);
	RegisterComponent<NinePatchRect>("NinePatchRect")
		.ResourceField("texture", &NinePatchRect::texture, "ImageTexture")
		.Field("paddingLeft", &NinePatchRect::paddingLeft)
		.Field("paddingRight", &NinePatchRect::paddingRight)
		.Field("paddingTop", &NinePatchRect::paddingTop)
		.Field("paddingBottom", &NinePatchRect::paddingBottom)
		.Field("tileCenter", &NinePatchRect::tileCenter)
		.Field("tileEdges", &NinePatchRect::tileEdges)
		.Field("drawCenter", &NinePatchRect::drawCenter)
		.Field("modulate", &NinePatchRect::modulate)
		.Field("size", &NinePatchRect::size)
		.Field("offset", &NinePatchRect::offset)
		.Field("centered", &NinePatchRect::centered);
	RegisterComponent<Text2D>("Text2D")
		.ResourceField("font", &Text2D::font, "Font")
		.Field("text", &Text2D::text)
		.Field("modulate", &Text2D::modulate)
		.Field("fontSize", &Text2D::fontSize)
		.Field("wrapWidth", &Text2D::wrapWidth)
		.Field("lineSpacing", &Text2D::lineSpacing)
		.Field("horizontalAlignment", &Text2D::horizontalAlignment)
		.Field("verticalAlignment", &Text2D::verticalAlignment)
		.Field("offset", &Text2D::offset);
	RegisterComponent<AnimatedSprite>("AnimatedSprite")
		.ResourceField("animations", &AnimatedSprite::animations, "SpriteSheetAnimation")
		.Field("currentAnimation", &AnimatedSprite::currentAnimation)
		.Field("autoplay", &AnimatedSprite::autoplay)
		.Field("speed", &AnimatedSprite::speed)
		.Field("modulate", &AnimatedSprite::modulate)
		.Field("size", &AnimatedSprite::size)
		.Field("offset", &AnimatedSprite::offset)
		.Field("centered", &AnimatedSprite::centered)
		.Field("isPlaying", &AnimatedSprite::isPlaying, FieldFlags::Expose)
		.Field("frame", &AnimatedSprite::frame, FieldFlags::Expose);
	RegisterComponent<AnimationPlayer>("AnimationPlayer")
		.ResourceField("library", &AnimationPlayer::library, "AnimationLibrary",
			FieldFlags::Serialize)
		.Field("currentAnimation", &AnimationPlayer::currentAnimation)
		.Field("isPlaying", &AnimationPlayer::isPlaying, FieldFlags::Expose)
		.Field("autoplay", &AnimationPlayer::autoplay)
		.Field("speed", &AnimationPlayer::speed);
	RegisterComponent<Tilemap>("Tilemap").Field("cellSize", &Tilemap::cellSize);
	// Register before systems to avoid implicit flecs registration. worldMatrix is derived.
	RegisterComponent<Transform2D>("Transform2D")
		.Field("position", &Transform2D::position)
		.Field("rotation", &Transform2D::rotation)
		.Field("scale", &Transform2D::scale);

	// Compute world transforms parent-first using the nearest ancestor with Transform2D.
	// Roots compose against identity. PostUpdate runs after gameplay and physics write-back
	// so rendering sees current poses.
	_world.system<Transform2D, const Transform2D *>("CalculateTransforms")
		.term_at(1)
		.parent()
		.cascade()
		.kind(flecs::PostUpdate)
		.each([](Transform2D &transform, const Transform2D *parent) {
			const glm::mat4 base = parent != nullptr ? parent->worldMatrix : glm::mat4(1.f);
			transform.worldMatrix = Matrix::CalculateTransform2D(transform.position,
				transform.rotation, transform.scale, Vector2(0.f, 0.f), base);
		});

	// No system of its own: a camera contributes a matrix on demand rather than
	// deriving per-entity state, so there is nothing for a pass to compute.
	RegisterComponent<Camera2D>("Camera2D")
		.Field("offset", &Camera2D::offset)
		.Field("rotatable", &Camera2D::rotatable)
		.Field("anchor", &Camera2D::anchor);

	// Engine infrastructure like Transform2D: the scene layer needs it before
	// any user code runs, and letting something else touch the type first
	// would have flecs auto-register it behind RegisterComponent's back.
	RegisterComponent<SceneRoot>("SceneRoot")
		.Field("sourcePath", &SceneRoot::sourcePath)
		.Field("activeCamera", &SceneRoot::activeCamera);

	// Registered despite having no fields: without a name it can't be written
	// to a scene file, so hiding an entity wouldn't survive a save/load. Its
	// *presence* is the whole payload.
	RegisterComponent<Hidden>("Hidden");

	RegisterComponent<RenderOrder>("RenderOrder")
		.Field("zIndex", &RenderOrder::zIndex)
		.Field("zIndexMode", &RenderOrder::zIndexMode)
		.Field("layer", &RenderOrder::layer)
		.Field("layerMode", &RenderOrder::layerMode);

	// Resolve layer and z-index together with parent-first traversal in PostUpdate.
	// They inherit independently; setting a layer does not reset relative z-index.
	_world.system<RenderOrder, const RenderOrder *>("CalculateRenderOrder")
		.term_at(1)
		.parent()
		.cascade()
		.kind(flecs::PostUpdate)
		.each([](RenderOrder &order, const RenderOrder *parent) {
			const i32 baseZ = parent != nullptr ? parent->effectiveZIndex : 0;
			order.effectiveZIndex =
				order.zIndexMode == ZIndexMode::Absolute ? order.zIndex : baseZ + order.zIndex;

			const i32 baseLayer = parent != nullptr ? parent->effectiveLayer : 0;
			order.effectiveLayer =
				order.layerMode == RenderLayerMode::Explicit ? order.layer : baseLayer;
		});
	RegisterParticles2D(*this);
	RegisterParallax2D(*this);
}

World::~World() = default;

void World::Progress(f32 deltaTime) {
	// flecs treats progress(0) as "measure wall-clock since last frame".
	// Callers pass an explicit dt; 0 must mean no time so tests and first
	// frames do not jump. A minimum finite value keeps flecs off that branch.
	const f32 dt = deltaTime == 0.f ? std::numeric_limits<f32>::min() : deltaTime;
	_world.progress(dt);
	flushDestroyed();
}

void World::SetPlayOnlySystemsEnabled(bool enabled) {
	_playOnlySystemsEnabled = enabled;
	for (System &system : _playOnlySystems) {
		if (!system.IsValid()) {
			continue;
		}
		system.SetEnabled(enabled);
	}
}

void World::QueueDestroy(Entity entity) {
	if (!entity.IsAlive()) {
		return;
	}
	for (const Entity &queued : _destroyQueue) {
		if (queued == entity) {
			return;
		}
	}
	_destroyQueue.push_back(entity);
}

void World::flushDestroyed() {
	// Swap so a QueueDestroy from a destructor can't invalidate the walk.
	std::vector<Entity> pending;
	pending.swap(_destroyQueue);
	for (Entity entity : pending) {
		entity.Destroy();
	}
}

f32 World::DeltaTime() const {
	return static_cast<f32>(_world.delta_time());
}

Entity World::CreateEntity() {
	return Entity(_world.entity());
}

EcsResult<ComponentType> World::RegisterComponent(const std::string &name,
	const std::vector<FieldDesc> &fields) {
	if (_componentsByName.find(name) != _componentsByName.end()) {
		return EcsResult<ComponentType>{ComponentType(), EcsError::DuplicateName};
	}

	flecs::entity componentEntity = _world.entity(name.c_str());
	for (const FieldDesc &field : fields) {
		// No explicit offset: there's no C++ struct to match, so flecs packs
		// the layout and RefreshFieldTable reads the offsets back out.
		AddComponentField(_world, componentEntity, field, 0, false);
	}
	if (fields.empty()) {
		// A zero-field type is still a component (a tag). Without EcsComponent
		// World::Find would hand the type entity back as a scene object, and
		// a root named the same thing would uniquify against it.
		componentEntity.set<flecs::Component>(flecs::Component{0, 0});
	}
	RefreshFieldTable(_world, componentEntity, fields);
	componentEntity.add<DataDescribed>();

	ComponentType type(componentEntity);
	_componentsByName.emplace(name, type);
	return EcsResult<ComponentType>{type, EcsError::Ok};
}

Entity World::findGroupEntity(const std::string &name) const {
	const flecs::entity scope = _world.lookup(kGroupScopeName);
	if (!scope) {
		return Entity();
	}

	const flecs::entity group = scope.lookup(name.c_str());
	if (!group) {
		return Entity();
	}

	return Entity(group);
}

std::vector<std::string> World::GetGroups() const {
	std::vector<std::string> groups;

	const flecs::entity scope = _world.lookup(kGroupScopeName);
	if (!scope) {
		return groups;
	}

	scope.children([&](flecs::entity group) {
		const char *name = group.name();
		if (name != nullptr) {
			groups.emplace_back(name);
		}
	});
	return groups;
}

Entity World::Find(const std::string &path) const {
	if (path.empty()) {
		return Entity();
	}

	const flecs::world_t *raw = _world.c_ptr();
	const flecs::entity_t found = ecs_lookup_path_w_sep(raw, 0, path.c_str(), "/", "/", false);
	if (found == 0) {
		return Entity();
	}

	flecs::entity entity(const_cast<flecs::world_t *>(raw), found);
	// Components and systems share the root namespace with entities; they're
	// not what a scene-path lookup is asking for. Neither is the scope that
	// owns group entities.
	if (entity.has<flecs::Component>() || entity.has(flecs::System)) {
		return Entity();
	}
	if (std::string(kGroupScopeName) == path) {
		return Entity();
	}

	return Entity(entity);
}

bool World::IsDeferred() const {
	return _world.is_deferred();
}

void World::SuspendDefer() {
	if (_immediate->active) {
		return;
	}
	flecs_suspend_readonly(_world.c_ptr(), &_immediate->state);
	_immediate->active = true;
}

void World::ResumeDefer() {
	if (!_immediate->active) {
		return;
	}
	flecs_resume_readonly(_world.c_ptr(), &_immediate->state);
	_immediate->active = false;
}

std::vector<ComponentType> World::Components() const {
	std::vector<ComponentType> types;
	for (const auto &[name, type] : _componentsByName) {
		types.push_back(type);
	}
	std::sort(types.begin(), types.end(),
		[](ComponentType a, ComponentType b) { return a.Name() < b.Name(); });
	return types;
}

ComponentType World::FindComponent(const std::string &name) const {
	const auto it = _componentsByName.find(name);
	if (it == _componentsByName.end()) {
		return ComponentType();
	}
	return it->second;
}

bool World::IsDataDescribed(ComponentType type) const {
	return type.IsValid() && type.Raw().has<DataDescribed>();
}

bool World::ComponentFieldsMatch(ComponentType type, const std::vector<FieldDesc> &fields) const {
	if (!type.IsValid()) {
		return false;
	}
	const std::vector<FieldInfo> &live = type.Fields();
	if (live.size() != fields.size()) {
		return false;
	}
	for (const FieldInfo &have : live) {
		bool found = false;
		for (const FieldDesc &want : fields) {
			if (have.name == want.name && have.type == want.type && have.flags == want.flags &&
				have.enumName == want.enumName) {
				found = true;
				break;
			}
		}
		if (!found) {
			return false;
		}
	}
	return true;
}

namespace {

struct SavedField {
	std::string name;
	FieldType type = FieldType::F32;
	std::string enumName;
	bool b = false;
	i32 i = 0;
	u32 u = 0;
	f32 f = 0.f;
	f64 d = 0.0;
	std::string s;
	Entity entity;
	ResourceID resource;
	Vector2 v2;
	Vector3 v3;
	Color color;
};

bool ReadSavedField(Entity entity, ComponentType type, const FieldInfo &info, SavedField &out) {
	FieldRef ref = entity.GetField(type, info.name);
	if (!ref) {
		return false;
	}
	out.name = info.name;
	out.type = info.type;
	out.enumName = info.enumName;
	switch (info.type) {
	case FieldType::Bool:
		return ref.Get(out.b);
	case FieldType::I32:
		return ref.Get(out.i);
	case FieldType::U32:
		return ref.Get(out.u);
	case FieldType::F32:
		return ref.Get(out.f);
	case FieldType::F64:
		return ref.Get(out.d);
	case FieldType::String:
		return ref.Get(out.s);
	case FieldType::EntityRef:
		return ref.Get(out.entity);
	case FieldType::ResourceRef:
		return ref.Get(out.resource);
	case FieldType::Enum:
		if (ref.Data() == nullptr) {
			return false;
		}
		out.i = *static_cast<const i32 *>(ref.Data());
		return true;
	case FieldType::Vector2:
		return ref.Get(out.v2);
	case FieldType::Vector3:
		return ref.Get(out.v3);
	case FieldType::Color:
		return ref.Get(out.color);
	}
	return false;
}

void WriteSavedField(Entity entity, ComponentType type, const SavedField &saved) {
	FieldRef ref = entity.GetField(type, saved.name);
	if (!ref || ref.Type() != saved.type) {
		return;
	}
	switch (saved.type) {
	case FieldType::Bool:
		ref.Set(saved.b);
		break;
	case FieldType::I32:
		ref.Set(saved.i);
		break;
	case FieldType::U32:
		ref.Set(saved.u);
		break;
	case FieldType::F32:
		ref.Set(saved.f);
		break;
	case FieldType::F64:
		ref.Set(saved.d);
		break;
	case FieldType::String:
		ref.Set(saved.s);
		break;
	case FieldType::EntityRef:
		ref.Set(saved.entity);
		break;
	case FieldType::ResourceRef:
		ref.Set(saved.resource);
		break;
	case FieldType::Enum:
		if (ref.Data() != nullptr && ref.Info() != nullptr &&
			saved.enumName == ref.Info()->enumName) {
			*static_cast<i32 *>(ref.Data()) = saved.i;
		}
		break;
	case FieldType::Vector2:
		ref.Set(saved.v2);
		break;
	case FieldType::Vector3:
		ref.Set(saved.v3);
		break;
	case FieldType::Color:
		ref.Set(saved.color);
		break;
	}
}

} // namespace

EcsResult<ComponentType> World::ReplaceDataDescribedFields(ComponentType type,
	const std::vector<FieldDesc> &fields) {
	if (!type.IsValid()) {
		return {ComponentType(), EcsError::NotDataDescribed};
	}
	if (!IsDataDescribed(type)) {
		return {type, EcsError::NotDataDescribed};
	}
	if (IsDeferred()) {
		Log::Error("ReplaceDataDescribedFields: refused while Progress is deferred");
		return {type, EcsError::Deferred};
	}
	if (ComponentFieldsMatch(type, fields)) {
		return {type, EcsError::Ok};
	}

	struct Holder {
		Entity entity;
		std::vector<SavedField> fields;
		bool live = false;
	};

	std::vector<Holder> holders;
	EachWithType(type, true, [&](Entity entity) {
		Holder holder;
		holder.entity = entity;
		holder.live = entity.Raw().has<ScriptLive>(type.Raw());
		for (const FieldInfo &info : type.Fields()) {
			SavedField saved;
			if (ReadSavedField(entity, type, info, saved)) {
				holder.fields.push_back(std::move(saved));
			}
		}
		holders.push_back(std::move(holder));
	});

	for (Holder &holder : holders) {
		if (holder.live) {
			holder.entity.Raw().remove<ScriptLive>(type.Raw());
		}
		holder.entity.Remove(type);
	}

	const std::string name = type.Name();
	_componentsByName.erase(name);
	type.Raw().destruct();

	EcsResult<ComponentType> created = RegisterComponent(name, fields);
	if (!created) {
		return created;
	}
	ComponentType next = created.value;

	for (Holder &holder : holders) {
		if (!holder.entity.IsAlive()) {
			continue;
		}
		holder.entity.Add(next);
		for (const SavedField &saved : holder.fields) {
			WriteSavedField(holder.entity, next, saved);
		}
		if (holder.live) {
			holder.entity.Raw().add<ScriptLive>(next.Raw());
		}
	}

	return {next, EcsError::Ok};
}

System World::RegisterUntypedSystem(const std::string &name, const std::vector<ComponentType> &with,
	const std::vector<ComponentType> &without, bool always, std::function<void(Entity)> callback) {
	auto builder = _world.system(name.c_str());
	for (const ComponentType &type : with) {
		if (type.IsValid()) {
			builder.with(type.Raw().id());
		}
	}
	for (const ComponentType &type : without) {
		if (type.IsValid()) {
			builder.without(type.Raw().id());
		}
	}

	flecs::system system;
	if (with.empty()) {
		// Custom run, not each: a query with no terms is not "once per
		// frame", it is "every entity". The run callback itself is invoked
		// once per tick; do not iterate it.
		system = builder.run([callback](flecs::iter &it) {
			(void)it;
			callback(Entity());
		});
	} else {
		system = builder.each([callback](flecs::entity entity) { callback(Entity(entity)); });
	}

	System handle(system);
	if (!always) {
		_playOnlySystems.push_back(handle);
		if (!_playOnlySystemsEnabled) {
			handle.SetEnabled(false);
		}
	}
	return handle;
}

void World::DestroyTrackedSystem(System system) {
	if (!system.IsValid()) {
		return;
	}
	const flecs::entity raw = system.Raw();
	_playOnlySystems.erase(
		std::remove_if(_playOnlySystems.begin(), _playOnlySystems.end(),
			[&](const System &tracked) { return !tracked.IsValid() || tracked.Raw() == raw; }),
		_playOnlySystems.end());
	system.Destroy();
}

void World::TrackPlayOnly(System system) {
	if (!system.IsValid()) {
		return;
	}
	_playOnlySystems.push_back(system);
	if (!_playOnlySystemsEnabled) {
		system.SetEnabled(false);
	}
}

} // namespace sowa
