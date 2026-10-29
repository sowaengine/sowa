#include "script/lua_bindings.hpp"
#include "animation/animated_sprite.hpp"
#include "animation/animation.hpp"
#include "particles/particles2d.hpp"
#include "script/lua_function.hpp"
#include "script/lua_physics.hpp"
#include "script/lua_tilemap.hpp"

#include <exception>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <fmt/args.h>
#include <fmt/format.h>

#include "audio/audio_mixer.hpp"
#include "core/input.hpp"
#include "core/input_binding.hpp"
#include "core/log.hpp"
#include "core/project_settings.hpp"
#include "core/random.hpp"
#include "core/reflection/field_ref.hpp"
#include "core/reflection/field_type.hpp"
#include "core/store.hpp"
#include "core/tween.hpp"
#include "ecs/component_type.hpp"
#include "ecs/entity.hpp"
#include "ecs/world.hpp"
#include "math/color.hpp"
#include "math/easing.hpp"
#include "math/math.hpp"
#include "math/rect.hpp"
#include "math/vector2.hpp"
#include "math/vector3.hpp"
#include "physics/physics_world.hpp"
#include "resource/resource.hpp"
#include "resource/resource_id.hpp"
#include "resource/resource_registry.hpp"
#include "scene/packed_scene.hpp"
#include "scene/scene.hpp"
#include "scene/scene_manager.hpp"
#include "script/lua_types.hpp"
#include "script/script_server.hpp"

namespace sowa {
namespace {

World *WorldOf(const Entity &entity) {
	if (!entity.IsAlive()) {
		return nullptr;
	}
	return static_cast<World *>(entity.Raw().world().get_ctx());
}

sol::object Nil(sol::this_state s) {
	return sol::make_object(s, sol::lua_nil);
}

sol::object PushEntity(sol::this_state s, Entity entity) {
	if (!entity.IsAlive()) {
		return Nil(s);
	}
	return sol::make_object(s, entity);
}

FieldRef Resolve(const FieldProxy &proxy) {
	Entity entity = proxy.entity;
	return entity.GetField(proxy.type, proxy.fieldName);
}

const FieldInfo *FindExposed(const ComponentView &view, std::string_view name) {
	const FieldInfo *info = view.type.FindField(name);
	if (info == nullptr || !info->IsExposed()) {
		return nullptr;
	}
	return info;
}

const FieldInfo *FindExposed(const Resource &resource, std::string_view name) {
	for (const FieldInfo &field : resource.Fields()) {
		if (field.name == name && field.IsExposed()) {
			return &field;
		}
	}
	return nullptr;
}

FieldRef FieldOn(Resource &resource, const FieldInfo &info) {
	return FieldRef(static_cast<u8 *>(static_cast<void *>(&resource)) + info.offset, &info);
}

Ref<Resource> LiveResource(ResourceRegistry *registry, ResourceID id) {
	if (registry == nullptr) {
		return nullptr;
	}
	return registry->Get(id);
}

// Keeps the Ref alive for the length of a field access so FieldRef cannot
// outlive the last owner if the registry drops the resource mid-call.
struct ResolvedResourceField {
	Ref<Resource> resource;
	FieldRef ref;

	explicit operator bool() const {
		return static_cast<bool>(ref);
	}
};

ResolvedResourceField ResolveLive(const ResourceFieldProxy &proxy) {
	ResolvedResourceField out;
	out.resource = LiveResource(proxy.registry, proxy.id);
	if (out.resource == nullptr) {
		return out;
	}
	const FieldInfo *info = FindExposed(*out.resource, proxy.fieldName);
	if (info == nullptr) {
		return out;
	}
	out.ref = FieldOn(*out.resource, *info);
	return out;
}

bool AsVector2(const sol::object &object, Vector2 &out) {
	if (object.is<Vector2>()) {
		out = object.as<Vector2>();
		return true;
	}
	if (object.is<FieldProxy>()) {
		Vector2 *value = Resolve(object.as<FieldProxy>()).As<Vector2>();
		if (value == nullptr) {
			return false;
		}
		out = *value;
		return true;
	}
	if (!object.is<ResourceFieldProxy>()) {
		return false;
	}
	ResolvedResourceField resolved = ResolveLive(object.as<ResourceFieldProxy>());
	Vector2 *value = resolved.ref.As<Vector2>();
	if (value == nullptr) {
		return false;
	}
	out = *value;
	return true;
}

bool AsVector3(const sol::object &object, Vector3 &out) {
	if (object.is<Vector3>()) {
		out = object.as<Vector3>();
		return true;
	}
	if (object.is<FieldProxy>()) {
		Vector3 *value = Resolve(object.as<FieldProxy>()).As<Vector3>();
		if (value == nullptr) {
			return false;
		}
		out = *value;
		return true;
	}
	if (!object.is<ResourceFieldProxy>()) {
		return false;
	}
	ResolvedResourceField resolved = ResolveLive(object.as<ResourceFieldProxy>());
	Vector3 *value = resolved.ref.As<Vector3>();
	if (value == nullptr) {
		return false;
	}
	out = *value;
	return true;
}

bool AsColor(const sol::object &object, Color &out) {
	if (object.is<Color>()) {
		out = object.as<Color>();
		return true;
	}
	if (object.is<FieldProxy>()) {
		Color *value = Resolve(object.as<FieldProxy>()).As<Color>();
		if (value == nullptr) {
			return false;
		}
		out = *value;
		return true;
	}
	if (!object.is<ResourceFieldProxy>()) {
		return false;
	}
	ResolvedResourceField resolved = ResolveLive(object.as<ResourceFieldProxy>());
	Color *value = resolved.ref.As<Color>();
	if (value == nullptr) {
		return false;
	}
	out = *value;
	return true;
}

bool IsLuaNumber(const sol::object &object) {
	return object.get_type() == sol::type::number;
}

sol::object ReadPrimitiveField(FieldRef ref, const FieldInfo &info, sol::this_state s) {
	if (!ref) {
		return Nil(s);
	}

	switch (info.type) {
	case FieldType::Bool: {
		const bool *value = ref.As<bool>();
		return value != nullptr ? sol::make_object(s, *value) : Nil(s);
	}
	case FieldType::I32: {
		const i32 *value = ref.As<i32>();
		return value != nullptr ? sol::make_object(s, *value) : Nil(s);
	}
	case FieldType::U32: {
		const u32 *value = ref.As<u32>();
		return value != nullptr ? sol::make_object(s, *value) : Nil(s);
	}
	case FieldType::F32: {
		const f32 *value = ref.As<f32>();
		return value != nullptr ? sol::make_object(s, *value) : Nil(s);
	}
	case FieldType::F64: {
		const f64 *value = ref.As<f64>();
		return value != nullptr ? sol::make_object(s, *value) : Nil(s);
	}
	case FieldType::String: {
		const std::string *value = ref.As<std::string>();
		return value != nullptr ? sol::make_object(s, *value) : Nil(s);
	}
	case FieldType::EntityRef: {
		const Entity *value = ref.As<Entity>();
		return value != nullptr ? PushEntity(s, *value) : Nil(s);
	}
	case FieldType::ResourceRef: {
		const ResourceID *value = ref.As<ResourceID>();
		return value != nullptr ? sol::make_object(s, value->value) : Nil(s);
	}
	case FieldType::Enum: {
		if (ref.Data() == nullptr || ref.Info() == nullptr) {
			return Nil(s);
		}
		const i32 value = *static_cast<const i32 *>(ref.Data());
		const std::string *name = ref.Info()->FindEnumName(value);
		if (name != nullptr) {
			return sol::make_object(s, *name);
		}
		return sol::make_object(s, value);
	}
	case FieldType::Vector2:
	case FieldType::Vector3:
	case FieldType::Color:
		return Nil(s);
	}
	return Nil(s);
}

sol::object ReadField(const ComponentView &view, const FieldInfo &info, sol::this_state s) {
	Entity entity = view.entity;
	FieldRef ref = entity.GetField(view.type, info.name);
	if (!ref) {
		return Nil(s);
	}

	switch (info.type) {
	case FieldType::Vector2:
	case FieldType::Vector3:
	case FieldType::Color:
		return sol::make_object(s, FieldProxy{view.entity, view.type, info.name});
	default:
		return ReadPrimitiveField(ref, info, s);
	}
}

bool WriteField(FieldRef ref, sol::object value) {
	if (!ref) {
		return false;
	}

	switch (ref.Type()) {
	case FieldType::Bool:
		if (value.get_type() != sol::type::boolean) {
			return false;
		}
		return ref.Set(value.as<bool>());
	case FieldType::I32:
		if (!IsLuaNumber(value)) {
			return false;
		}
		return ref.Set(value.as<i32>());
	case FieldType::U32:
		if (!IsLuaNumber(value)) {
			return false;
		}
		return ref.Set(value.as<u32>());
	case FieldType::F32:
		if (!IsLuaNumber(value)) {
			return false;
		}
		return ref.Set(value.as<f32>());
	case FieldType::F64:
		if (!IsLuaNumber(value)) {
			return false;
		}
		return ref.Set(value.as<f64>());
	case FieldType::String:
		if (value.get_type() != sol::type::string) {
			return false;
		}
		return ref.Set(value.as<std::string>());
	case FieldType::EntityRef:
		if (value.get_type() == sol::type::lua_nil) {
			return ref.Set(Entity());
		}
		if (!value.is<Entity>()) {
			return false;
		}
		return ref.Set(value.as<Entity>());
	case FieldType::ResourceRef: {
		ResourceID id;
		if (value.get_type() == sol::type::lua_nil) {
			id.value = 0;
		} else if (IsLuaNumber(value)) {
			id.value = value.as<u64>();
		} else {
			return false;
		}
		return ref.Set(id);
	}
	case FieldType::Enum: {
		if (ref.Data() == nullptr || ref.Info() == nullptr) {
			return false;
		}
		i32 out = 0;
		if (value.get_type() == sol::type::string) {
			if (!ref.Info()->FindEnumValue(value.as<std::string>(), out)) {
				Log::Warn("Script: unknown enum value '{}'", value.as<std::string>());
				return false;
			}
		} else if (IsLuaNumber(value)) {
			out = value.as<i32>();
		} else {
			return false;
		}
		*static_cast<i32 *>(ref.Data()) = out;
		return true;
	}
	case FieldType::Vector2: {
		Vector2 vec;
		if (!AsVector2(value, vec)) {
			return false;
		}
		return ref.Set(vec);
	}
	case FieldType::Vector3: {
		Vector3 vec;
		if (!AsVector3(value, vec)) {
			return false;
		}
		return ref.Set(vec);
	}
	case FieldType::Color: {
		Color color;
		if (!AsColor(value, color)) {
			return false;
		}
		return ref.Set(color);
	}
	}
	return false;
}

ScriptServer *ServerOf(sol::this_state s) {
	sol::state_view lua(s);
	sol::object object = lua.registry()["sowa_scripts"];
	if (!object.valid() || object.get_type() != sol::type::lightuserdata) {
		return nullptr;
	}
	return static_cast<ScriptServer *>(object.as<void *>());
}

AudioMixer *MixerOf(sol::this_state s) {
	sol::state_view lua(s);
	sol::object object = lua.registry()["sowa_audio_mixer"];
	if (!object.valid() || object.get_type() != sol::type::lightuserdata) {
		return nullptr;
	}
	return static_cast<AudioMixer *>(object.as<void *>());
}

PhysicsWorld *PhysicsOf(sol::this_state s) {
	sol::state_view lua(s);
	sol::object object = lua.registry()["sowa_physics"];
	if (!object.valid() || object.get_type() != sol::type::lightuserdata) {
		return nullptr;
	}
	return static_cast<PhysicsWorld *>(object.as<void *>());
}

sol::object AudioSourceMethod(ComponentView &view, const std::string &name, sol::this_state s) {
	if (view.type.Name() != "AudioSource") {
		return Nil(s);
	}

	AudioMixer *mixer = MixerOf(s);
	if (name == "play") {
		return MakeLuaFunction(s, [mixer](ComponentView self) {
			if (mixer != nullptr) {
				mixer->Play(self.entity);
			}
		});
	}
	if (name == "pause") {
		return MakeLuaFunction(s, [mixer](ComponentView self) {
			if (mixer != nullptr) {
				mixer->Pause(self.entity);
			}
		});
	}
	if (name == "stop") {
		return MakeLuaFunction(s, [mixer](ComponentView self) {
			if (mixer != nullptr) {
				mixer->Stop(self.entity);
			}
		});
	}
	if (name == "is_playing") {
		return MakeLuaFunction(s, [mixer](ComponentView self) {
			return mixer != nullptr && mixer->IsPlaying(self.entity);
		});
	}
	if (name == "is_paused") {
		return MakeLuaFunction(s, [mixer](ComponentView self) {
			return mixer != nullptr && mixer->IsPaused(self.entity);
		});
	}
	return Nil(s);
}

sol::object RigidBody2DMethod(ComponentView &view, const std::string &name, sol::this_state s) {
	if (view.type.Name() != "RigidBody2D") {
		return Nil(s);
	}

	PhysicsWorld *physics = PhysicsOf(s);
	if (name == "apply_force") {
		return MakeLuaFunction(s, [physics](ComponentView self, Vector2 force) {
			if (physics != nullptr) {
				physics->ApplyForce(self.entity, force);
			}
		});
	}
	if (name == "apply_torque") {
		return MakeLuaFunction(s, [physics](ComponentView self, f32 torque) {
			if (physics != nullptr) {
				physics->ApplyTorque(self.entity, torque);
			}
		});
	}
	if (name == "apply_impulse") {
		return MakeLuaFunction(s, [physics](ComponentView self, Vector2 impulse) {
			if (physics != nullptr) {
				physics->ApplyImpulse(self.entity, impulse);
			}
		});
	}
	if (name == "apply_angular_impulse") {
		return MakeLuaFunction(s, [physics](ComponentView self, f32 impulse) {
			if (physics != nullptr) {
				physics->ApplyAngularImpulse(self.entity, impulse);
			}
		});
	}
	if (name == "set_linear_velocity") {
		return MakeLuaFunction(s, [physics](ComponentView self, Vector2 velocity) {
			if (physics != nullptr) {
				physics->SetLinearVelocity(self.entity, velocity);
			}
		});
	}
	if (name == "linear_velocity") {
		return MakeLuaFunction(s, [physics](ComponentView self) {
			return physics != nullptr ? physics->GetLinearVelocity(self.entity) : Vector2();
		});
	}
	if (name == "set_angular_velocity") {
		return MakeLuaFunction(s, [physics](ComponentView self, f32 degreesPerSecond) {
			if (physics != nullptr) {
				physics->SetAngularVelocity(self.entity, degreesPerSecond);
			}
		});
	}
	if (name == "angular_velocity") {
		return MakeLuaFunction(s, [physics](ComponentView self) {
			return physics != nullptr ? physics->GetAngularVelocity(self.entity) : 0.f;
		});
	}
	return Nil(s);
}

sol::object ComponentIndex(ComponentView &view, sol::stack_object key, sol::this_state s) {
	if (!key.is<std::string>()) {
		return Nil(s);
	}
	const std::string name = key.as<std::string>();
	const FieldInfo *info = FindExposed(view, name);
	if (info != nullptr) {
		return ReadField(view, *info, s);
	}
	sol::object method = TilemapMethod(view, name, s);
	if (method.get_type() != sol::type::lua_nil) {
		return method;
	}
	method = RaycastMethod(view, name, s);
	if (method.get_type() != sol::type::lua_nil) {
		return method;
	}
	method = AudioSourceMethod(view, name, s);
	if (method.get_type() != sol::type::lua_nil) {
		return method;
	}
	method = RigidBody2DMethod(view, name, s);
	if (method.get_type() != sol::type::lua_nil) {
		return method;
	}
	ScriptServer *scripts = ServerOf(s);
	if (scripts == nullptr) {
		return Nil(s);
	}
	return scripts->IndexComponent(view.entity, view.type, name, s);
}

void ComponentNewIndex(ComponentView &view, sol::stack_object key, sol::stack_object value) {
	if (!key.is<std::string>()) {
		return;
	}
	const std::string name = key.as<std::string>();
	const FieldInfo *info = FindExposed(view, name);
	if (info != nullptr) {
		FieldRef ref = view.entity.GetField(view.type, info->name);
		ValidatePhysicsField(view, name, sol::object(value));
		WriteField(ref, sol::object(value));
		return;
	}
	ScriptServer *scripts = ServerOf(sol::this_state{value.lua_state()});
	if (scripts != nullptr) {
		scripts->NewIndexComponent(view.entity, view.type, name, sol::object(value));
	}
}

sol::object CompoundIndex(FieldRef ref, const std::string &name, sol::this_state s) {
	if (!ref) {
		return sol::make_object(s, 0.f);
	}

	switch (ref.Type()) {
	case FieldType::Vector2: {
		const Vector2 *value = ref.As<Vector2>();
		if (value == nullptr) {
			return sol::make_object(s, 0.f);
		}
		if (name == "x") {
			return sol::make_object(s, value->x);
		}
		if (name == "y") {
			return sol::make_object(s, value->y);
		}
		return Nil(s);
	}
	case FieldType::Vector3: {
		const Vector3 *value = ref.As<Vector3>();
		if (value == nullptr) {
			return sol::make_object(s, 0.f);
		}
		if (name == "x") {
			return sol::make_object(s, value->x);
		}
		if (name == "y") {
			return sol::make_object(s, value->y);
		}
		if (name == "z") {
			return sol::make_object(s, value->z);
		}
		return Nil(s);
	}
	case FieldType::Color: {
		const Color *value = ref.As<Color>();
		if (value == nullptr) {
			return sol::make_object(s, 0.f);
		}
		if (name == "r") {
			return sol::make_object(s, value->r);
		}
		if (name == "g") {
			return sol::make_object(s, value->g);
		}
		if (name == "b") {
			return sol::make_object(s, value->b);
		}
		if (name == "a") {
			return sol::make_object(s, value->a);
		}
		return Nil(s);
	}
	default:
		return Nil(s);
	}
}

bool CompoundNewIndex(FieldRef ref, const std::string &name, f32 number) {
	if (!ref) {
		return false;
	}

	switch (ref.Type()) {
	case FieldType::Vector2: {
		Vector2 *vec = ref.As<Vector2>();
		if (vec == nullptr) {
			return false;
		}
		if (name == "x") {
			vec->x = number;
		} else if (name == "y") {
			vec->y = number;
		} else {
			return false;
		}
		return true;
	}
	case FieldType::Vector3: {
		Vector3 *vec = ref.As<Vector3>();
		if (vec == nullptr) {
			return false;
		}
		if (name == "x") {
			vec->x = number;
		} else if (name == "y") {
			vec->y = number;
		} else if (name == "z") {
			vec->z = number;
		} else {
			return false;
		}
		return true;
	}
	case FieldType::Color: {
		Color *color = ref.As<Color>();
		if (color == nullptr) {
			return false;
		}
		if (name == "r") {
			color->r = number;
		} else if (name == "g") {
			color->g = number;
		} else if (name == "b") {
			color->b = number;
		} else if (name == "a") {
			color->a = number;
		} else {
			return false;
		}
		return true;
	}
	default:
		return false;
	}
}

sol::object ProxyIndex(FieldProxy &proxy, sol::stack_object key, sol::this_state s) {
	if (!key.is<std::string>()) {
		return sol::make_object(s, 0.f);
	}
	return CompoundIndex(Resolve(proxy), key.as<std::string>(), s);
}

void ProxyNewIndex(FieldProxy &proxy, sol::stack_object key, sol::stack_object value) {
	if (!key.is<std::string>() || !IsLuaNumber(sol::object(value))) {
		return;
	}
	CompoundNewIndex(Resolve(proxy), key.as<std::string>(), sol::object(value).as<f32>());
}

sol::object ResourceIndex(ResourceView &view, sol::stack_object key, sol::this_state s) {
	if (!key.is<std::string>()) {
		return Nil(s);
	}
	Ref<Resource> resource = LiveResource(view.registry, view.id);
	if (resource == nullptr) {
		return Nil(s);
	}
	const FieldInfo *info = FindExposed(*resource, key.as<std::string>());
	if (info == nullptr) {
		return Nil(s);
	}
	switch (info->type) {
	case FieldType::Vector2:
	case FieldType::Vector3:
	case FieldType::Color:
		return sol::make_object(s, ResourceFieldProxy{view.registry, view.id, info->name});
	default:
		return ReadPrimitiveField(FieldOn(*resource, *info), *info, s);
	}
}

void ResourceNewIndex(ResourceView &view, sol::stack_object key, sol::stack_object value) {
	if (!key.is<std::string>() || view.registry == nullptr) {
		return;
	}
	Ref<Resource> resource = LiveResource(view.registry, view.id);
	if (resource == nullptr) {
		return;
	}
	const FieldInfo *info = FindExposed(*resource, key.as<std::string>());
	if (info == nullptr) {
		return;
	}
	if (!WriteField(FieldOn(*resource, *info), sol::object(value))) {
		return;
	}
	view.registry->Reload(view.id);
}

sol::object ResourceProxyIndex(ResourceFieldProxy &proxy, sol::stack_object key,
	sol::this_state s) {
	if (!key.is<std::string>()) {
		return sol::make_object(s, 0.f);
	}
	ResolvedResourceField resolved = ResolveLive(proxy);
	return CompoundIndex(resolved.ref, key.as<std::string>(), s);
}

void ResourceProxyNewIndex(ResourceFieldProxy &proxy, sol::stack_object key,
	sol::stack_object value) {
	if (!key.is<std::string>() || !IsLuaNumber(sol::object(value))) {
		return;
	}
	ResolvedResourceField resolved = ResolveLive(proxy);
	if (!resolved) {
		return;
	}
	if (!CompoundNewIndex(resolved.ref, key.as<std::string>(), sol::object(value).as<f32>())) {
		return;
	}
	if (proxy.registry != nullptr) {
		proxy.registry->Reload(proxy.id);
	}
}

sol::object VecAdd(sol::this_state s, sol::object a, sol::object b) {
	Vector2 va, vb;
	if (AsVector2(a, va) && AsVector2(b, vb)) {
		return sol::make_object(s, va + vb);
	}
	Vector3 a3, b3;
	if (AsVector3(a, a3) && AsVector3(b, b3)) {
		return sol::make_object(s, a3 + b3);
	}
	luaL_error(s, "invalid '+' operands");
	return Nil(s);
}

sol::object VecSub(sol::this_state s, sol::object a, sol::object b) {
	Vector2 va, vb;
	if (AsVector2(a, va) && AsVector2(b, vb)) {
		return sol::make_object(s, va - vb);
	}
	Vector3 a3, b3;
	if (AsVector3(a, a3) && AsVector3(b, b3)) {
		return sol::make_object(s, a3 - b3);
	}
	luaL_error(s, "invalid '-' operands");
	return Nil(s);
}

sol::object VecMul(sol::this_state s, sol::object a, sol::object b) {
	Vector2 va, vb;
	if (AsVector2(a, va) && AsVector2(b, vb)) {
		return sol::make_object(s, va * vb);
	}
	if (AsVector2(a, va) && IsLuaNumber(b)) {
		return sol::make_object(s, va * b.as<f32>());
	}
	if (IsLuaNumber(a) && AsVector2(b, vb)) {
		return sol::make_object(s, vb * a.as<f32>());
	}
	Vector3 a3, b3;
	if (AsVector3(a, a3) && AsVector3(b, b3)) {
		return sol::make_object(s, a3 * b3);
	}
	if (AsVector3(a, a3) && IsLuaNumber(b)) {
		return sol::make_object(s, a3 * b.as<f32>());
	}
	if (IsLuaNumber(a) && AsVector3(b, b3)) {
		return sol::make_object(s, b3 * a.as<f32>());
	}
	luaL_error(s, "invalid '*' operands");
	return Nil(s);
}

sol::object VecDiv(sol::this_state s, sol::object a, sol::object b) {
	Vector2 va, vb;
	if (AsVector2(a, va) && AsVector2(b, vb)) {
		return sol::make_object(s, va / vb);
	}
	if (AsVector2(a, va) && IsLuaNumber(b)) {
		return sol::make_object(s, va / b.as<f32>());
	}
	Vector3 a3, b3;
	if (AsVector3(a, a3) && AsVector3(b, b3)) {
		return sol::make_object(s, a3 / b3);
	}
	if (AsVector3(a, a3) && IsLuaNumber(b)) {
		return sol::make_object(s, a3 / b.as<f32>());
	}
	luaL_error(s, "invalid '/' operands");
	return Nil(s);
}

sol::object VecUnm(sol::this_state s, sol::object a) {
	Vector2 v;
	if (AsVector2(a, v)) {
		return sol::make_object(s, -v);
	}
	Vector3 v3;
	if (AsVector3(a, v3)) {
		return sol::make_object(s, -v3);
	}
	luaL_error(s, "invalid unary '-' operand");
	return Nil(s);
}

void BindVector2(sol::state &lua) {
	lua.new_usertype<Vector2>("Vector2", sol::call_constructor,
		sol::constructors<Vector2(), Vector2(f32), Vector2(f32, f32)>(), "x", &Vector2::x, "y",
		&Vector2::y, "length", &Vector2::Length, "length_squared", &Vector2::LengthSquared,
		"distance_to", &Vector2::DistanceTo, "normalize", &Vector2::Normalize, "normalized",
		&Vector2::Normalized, sol::meta_function::addition, &VecAdd,
		sol::meta_function::subtraction, &VecSub, sol::meta_function::multiplication, &VecMul,
		sol::meta_function::division, &VecDiv, sol::meta_function::unary_minus, &VecUnm,
		sol::meta_function::equal_to, &Vector2::operator==, sol::meta_function::to_string,
		[](const Vector2 &v) { return fmt::format("Vector2({}, {})", v.x, v.y); });
}

void BindRect(sol::state &lua) {
	lua.new_usertype<Rect>("Rect", sol::call_constructor,
		sol::constructors<Rect(), Rect(f32, f32), Rect(f32, f32, f32, f32)>(), "x", &Rect::x, "y",
		&Rect::y, "w", &Rect::w, "h", &Rect::h, "left", &Rect::Left, "right", &Rect::Right,
		"bottom", &Rect::Bottom, "top", &Rect::Top, "contains", &Rect::Contains, "intersects",
		&Rect::Intersects, "map_point", &Rect::MapPoint, sol::meta_function::to_string,
		[](const Rect &r) { return fmt::format("Rect({}, {}, {}, {})", r.x, r.y, r.w, r.h); });
}

void BindVector3(sol::state &lua) {
	lua.new_usertype<Vector3>("Vector3", sol::call_constructor,
		sol::constructors<Vector3(), Vector3(f32), Vector3(f32, f32, f32)>(), "x", &Vector3::x, "y",
		&Vector3::y, "z", &Vector3::z, "length", &Vector3::Length, "length_squared",
		&Vector3::LengthSquared, "distance_to", &Vector3::DistanceTo, "normalize",
		&Vector3::Normalize, "normalized", &Vector3::Normalized, sol::meta_function::addition,
		&VecAdd, sol::meta_function::subtraction, &VecSub, sol::meta_function::multiplication,
		&VecMul, sol::meta_function::division, &VecDiv, sol::meta_function::unary_minus, &VecUnm,
		sol::meta_function::equal_to, &Vector3::operator==, sol::meta_function::to_string,
		[](const Vector3 &v) { return fmt::format("Vector3({}, {}, {})", v.x, v.y, v.z); });
}

Color ColorRgb(u8 r, u8 g, u8 b, sol::optional<u8> a) {
	return Color::RGB(r, g, b, a.value_or(255));
}

void BindColor(sol::state &lua) {
	lua.new_usertype<Color>("Color", sol::call_constructor,
		sol::constructors<Color(), Color(f32), Color(f32, f32, f32), Color(f32, f32, f32, f32)>(),
		"r", &Color::r, "g", &Color::g, "b", &Color::b, "a", &Color::a, "rgb", &ColorRgb, "hsv",
		&Color::HSV, sol::meta_function::equal_to, &Color::operator==,
		sol::meta_function::to_string,
		[](const Color &c) { return fmt::format("Color({}, {}, {}, {})", c.r, c.g, c.b, c.a); });
}

sol::object MathLerp(sol::object a, sol::object b, f32 t, sol::optional<i32> easing,
	sol::this_state s) {
	if (easing) {
		t = Math::Ease(t, static_cast<Easing>(*easing));
	}
	if (IsLuaNumber(a) && IsLuaNumber(b)) {
		return sol::make_object(s, Math::Lerp(a.as<f32>(), b.as<f32>(), t));
	}
	Vector2 a2, b2;
	if (AsVector2(a, a2) && AsVector2(b, b2)) {
		return sol::make_object(s, Math::Lerp(a2, b2, t));
	}
	Vector3 a3, b3;
	if (AsVector3(a, a3) && AsVector3(b, b3)) {
		return sol::make_object(s, Math::Lerp(a3, b3, t));
	}
	Color ac, bc;
	if (AsColor(a, ac) && AsColor(b, bc)) {
		return sol::make_object(s, Math::Lerp(ac, bc, t));
	}
	Log::Warn("math.lerp: expected number, Vector2, Vector3, or Color");
	return Nil(s);
}

Random &LuaRng() {
	static Random rng;
	return rng;
}

// Match Lua's float [0, 1) and inclusive integer range semantics.
sol::object MathRandom(sol::optional<f32> min, sol::optional<f32> max, sol::this_state s) {
	if (!min) {
		return sol::make_object(s, LuaRng().Float());
	}
	if (!max) {
		return sol::make_object(s, LuaRng().Range(1, static_cast<i32>(*min)));
	}
	return sol::make_object(s, LuaRng().Range(static_cast<i32>(*min), static_cast<i32>(*max)));
}

void BindMath(sol::state &lua) {
	sol::table math = lua.create_named_table("math");
	math["pi"] = Math::Pi;
	math["abs"] = &Math::Abs;
	math["sin"] = &Math::Sin;
	math["cos"] = &Math::Cos;
	math["sqrt"] = &Math::Sqrt;
	math["atan2"] = &Math::Atan2;
	math["clamp"] = [](f32 value, f32 min, f32 max) { return Math::Clamp(value, min, max); };
	math["map"] = [](f32 value, f32 inMin, f32 inMax, f32 outMin, f32 outMax) {
		return Math::Map(value, inMin, inMax, outMin, outMax);
	};
	math["tween_map"] = [](f32 value, f32 outMin, f32 outMax) {
		return Math::TweenMap(value, outMin, outMax);
	};
	math["ease"] = [](f32 t, i32 easing) { return Math::Ease(t, static_cast<Easing>(easing)); };
	math["lerp"] = &MathLerp;
	math["random"] = &MathRandom;
}

void BindEasing(sol::state &lua) {
	sol::table easing = lua.create_named_table("Easing");
	auto set = [&](const char *name, Easing value) { easing[name] = static_cast<i32>(value); };
	set("Linear", Easing::Linear);
	set("SineIn", Easing::SineIn);
	set("SineOut", Easing::SineOut);
	set("SineInOut", Easing::SineInOut);
	set("CubicIn", Easing::CubicIn);
	set("CubicOut", Easing::CubicOut);
	set("CubicInOut", Easing::CubicInOut);
	set("QuintIn", Easing::QuintIn);
	set("QuintOut", Easing::QuintOut);
	set("QuintInOut", Easing::QuintInOut);
	set("CircIn", Easing::CircIn);
	set("CircOut", Easing::CircOut);
	set("CircInOut", Easing::CircInOut);
	set("ElasticIn", Easing::ElasticIn);
	set("ElasticOut", Easing::ElasticOut);
	set("ElasticInOut", Easing::ElasticInOut);
	set("QuadIn", Easing::QuadIn);
	set("QuadOut", Easing::QuadOut);
	set("QuadInOut", Easing::QuadInOut);
	set("QuartIn", Easing::QuartIn);
	set("QuartOut", Easing::QuartOut);
	set("QuartInOut", Easing::QuartInOut);
	set("ExpoIn", Easing::ExpoIn);
	set("ExpoOut", Easing::ExpoOut);
	set("ExpoInOut", Easing::ExpoInOut);
	set("BackIn", Easing::BackIn);
	set("BackOut", Easing::BackOut);
	set("BackInOut", Easing::BackInOut);
	set("BounceIn", Easing::BounceIn);
	set("BounceOut", Easing::BounceOut);
	set("BounceInOut", Easing::BounceInOut);
}

std::string FormatEntity(const Entity &entity) {
	if (!entity.IsAlive()) {
		return std::string("<invalid entity>");
	}
	const std::string path = entity.GetPath();
	return path.empty() ? std::string("<unnamed entity>") : path;
}

std::string FormatCompound(FieldRef ref) {
	if (const Vector2 *v = ref.As<Vector2>()) {
		return fmt::format("Vector2({}, {})", v->x, v->y);
	}
	if (const Vector3 *v = ref.As<Vector3>()) {
		return fmt::format("Vector3({}, {}, {})", v->x, v->y, v->z);
	}
	if (const Color *c = ref.As<Color>()) {
		return fmt::format("Color({}, {}, {}, {})", c->r, c->g, c->b, c->a);
	}
	return std::string("<invalid field>");
}

void BindFieldProxy(sol::state &lua) {
	lua.new_usertype<FieldProxy>("FieldProxy", sol::no_constructor, sol::meta_function::index,
		&ProxyIndex, sol::meta_function::new_index, &ProxyNewIndex, sol::meta_function::addition,
		&VecAdd, sol::meta_function::subtraction, &VecSub, sol::meta_function::multiplication,
		&VecMul, sol::meta_function::division, &VecDiv, sol::meta_function::unary_minus, &VecUnm,
		sol::meta_function::to_string,
		[](const FieldProxy &proxy) { return FormatCompound(Resolve(proxy)); });
}

void BindResourceFieldProxy(sol::state &lua) {
	lua.new_usertype<ResourceFieldProxy>("ResourceFieldProxy", sol::no_constructor,
		sol::meta_function::index, &ResourceProxyIndex, sol::meta_function::new_index,
		&ResourceProxyNewIndex, sol::meta_function::addition, &VecAdd,
		sol::meta_function::subtraction, &VecSub, sol::meta_function::multiplication, &VecMul,
		sol::meta_function::division, &VecDiv, sol::meta_function::unary_minus, &VecUnm,
		sol::meta_function::to_string, [](const ResourceFieldProxy &proxy) {
			ResolvedResourceField resolved = ResolveLive(proxy);
			return FormatCompound(resolved.ref);
		});
}

void BindComponentView(sol::state &lua) {
	lua.new_usertype<ComponentView>("ComponentView", sol::no_constructor, sol::meta_function::index,
		&ComponentIndex, sol::meta_function::new_index, &ComponentNewIndex,
		sol::meta_function::to_string, [](const ComponentView &view) {
			const std::string owner = view.entity.IsAlive() && !view.entity.Has(view.type)
										  ? std::string("<removed>")
										  : FormatEntity(view.entity);
			return fmt::format("{}({})", view.type.Name(), owner);
		});
}

void BindResourceView(sol::state &lua) {
	lua.new_usertype<ResourceView>(
		"ResourceView", sol::no_constructor, sol::meta_function::index, &ResourceIndex,
		sol::meta_function::new_index, &ResourceNewIndex, sol::meta_function::equal_to,
		[](const ResourceView &a, const ResourceView &b) {
			return a.registry == b.registry && a.id == b.id;
		},
		sol::meta_function::to_string,
		[](const ResourceView &view) {
			if (view.registry == nullptr || !view.registry->Has(view.id)) {
				return std::string("<invalid resource>");
			}
			return fmt::format("{}({})", view.registry->TypeNameOf(view.id), view.id.value);
		});
}

sol::object EntityGet(Entity &entity, const std::string &typeName, sol::this_state s) {
	World *world = WorldOf(entity);
	if (world == nullptr) {
		return Nil(s);
	}
	ComponentType type = world->FindComponent(typeName);
	if (!type.IsValid() || !entity.Has(type)) {
		return Nil(s);
	}
	return sol::make_object(s, ComponentView{entity, type});
}

bool EntityHas(const Entity &entity, const std::string &typeName) {
	World *world = WorldOf(entity);
	if (world == nullptr) {
		return false;
	}
	ComponentType type = world->FindComponent(typeName);
	return type.IsValid() && entity.Has(type);
}

sol::object EntityAdd(Entity &entity, const std::string &typeName, sol::this_state s) {
	World *world = WorldOf(entity);
	if (world == nullptr) {
		return Nil(s);
	}
	ComponentType type = world->FindComponent(typeName);
	if (!type.IsValid()) {
		Log::Warn("Script: unknown component type '{}'", typeName);
		return Nil(s);
	}
	if (entity.Has(type)) {
		return sol::make_object(s, ComponentView{entity, type});
	}

	// Systems queue structural changes. ApplyDefaults writes fields, so the
	// add must land now - the same SuspendDefer Instantiates uses.
	const bool deferred = world->IsDeferred();
	if (deferred) {
		world->SuspendDefer();
	}
	entity.Add(type);
	ScriptServer *scripts = ServerOf(s);
	if (scripts != nullptr) {
		scripts->ApplyDefaults(entity, type);
	}
	if (deferred) {
		world->ResumeDefer();
	}
	return sol::make_object(s, ComponentView{entity, type});
}

void EntityRemove(Entity &entity, const std::string &typeName) {
	World *world = WorldOf(entity);
	if (world == nullptr) {
		return;
	}
	ComponentType type = world->FindComponent(typeName);
	if (type.IsValid()) {
		entity.Remove(type);
	}
}

sol::object EntityFind(const Entity &entity, const std::string &path, sol::this_state s) {
	return PushEntity(s, entity.Find(path));
}

sol::object EntityParent(const Entity &entity, sol::this_state s) {
	return PushEntity(s, entity.GetParent());
}

void EntitySetParent(Entity &entity, sol::object value) {
	if (!value.valid() || value.get_type() == sol::type::lua_nil) {
		entity.ClearParent();
		return;
	}
	if (value.is<Entity>()) {
		entity.SetParent(value.as<Entity>());
	} else {
		Log::Warn("Script: entity.parent expected Entity or nil");
	}
}

sol::as_table_t<std::vector<Entity>> EntityChildren(const Entity &entity) {
	if (!entity.IsAlive()) {
		return sol::as_table(std::vector<Entity>{});
	}
	return sol::as_table(entity.GetChildren());
}

void BindEntity(sol::state &lua) {
	lua.new_usertype<Entity>("Entity", sol::no_constructor, "is_alive", &Entity::IsAlive, "name",
		sol::property(&Entity::GetName,
			[](Entity &entity, std::string_view name) { entity.SetName(name); }),
		"path", sol::readonly_property(&Entity::GetPath), "parent",
		sol::property(&EntityParent, &EntitySetParent), "visible",
		sol::property(&Entity::IsVisible, &Entity::SetVisible), "find", &EntityFind, "children",
		&EntityChildren, "queue_destroy", &Entity::QueueDestroy, "destroy", &Entity::Destroy, "get",
		&EntityGet, "has", &EntityHas, "add", &EntityAdd, "remove", &EntityRemove,
		sol::meta_function::equal_to, &Entity::operator==, sol::meta_function::to_string,
		&FormatEntity);
}

sol::object WorldFind(const World &world, const std::string &path, sol::this_state s) {
	return PushEntity(s, world.Find(path));
}

Entity WorldCreateEntity(World &world, sol::optional<std::string> name) {
	Entity entity = world.CreateEntity();
	if (name && !name->empty()) {
		entity.SetName(*name);
	}
	return entity;
}

struct LuaSystemBuilder {
	ScriptServer *server = nullptr;
	std::string name;
	std::vector<std::string> with;
	std::vector<std::string> without;
	bool always = false;

	LuaSystemBuilder &With(sol::variadic_args args) {
		for (auto arg : args) {
			if (arg.is<std::string>()) {
				with.push_back(arg.as<std::string>());
			}
		}
		return *this;
	}

	LuaSystemBuilder &Without(sol::variadic_args args) {
		for (auto arg : args) {
			if (arg.is<std::string>()) {
				without.push_back(arg.as<std::string>());
			}
		}
		return *this;
	}

	LuaSystemBuilder &Always() {
		always = true;
		return *this;
	}

	void Each(sol::protected_function fn) {
		if (server != nullptr) {
			server->FinishSystem(name, with, without, always, std::move(fn));
		}
	}
};

void BindLuaSystemBuilder(sol::state &lua) {
	lua.new_usertype<LuaSystemBuilder>("LuaSystemBuilder", sol::no_constructor, "with",
		&LuaSystemBuilder::With, "without", &LuaSystemBuilder::Without, "always",
		&LuaSystemBuilder::Always, "each", &LuaSystemBuilder::Each);
}

void BindWorld(sol::state &lua, World &world, ScriptServer &scripts) {
	lua.new_usertype<World>(
		"World", sol::no_constructor, "find", &WorldFind, "create_entity", &WorldCreateEntity,
		"delta_time", sol::readonly_property(&World::DeltaTime), "component",
		[&scripts](World &, const std::string &name, sol::optional<sol::table> fields,
			sol::this_state s) { return scripts.DefineComponent(name, fields, s); },
		"system",
		[&scripts](World &, const std::string &name) {
			return LuaSystemBuilder{&scripts, name, {}, {}, false};
		});
	lua["world"] = &world;
}

void BindKeyTable(sol::state &lua) {
	sol::table key = lua.create_named_table("Key");
	key["Unknown"] = static_cast<i32>(Key::Unknown);
	for (const EnumConstant &constant : KeyConstants()) {
		key[constant.name] = constant.value;
	}
}

int LuaLog(lua_State *L, Log::Severity severity) {
	if (lua_gettop(L) < 1) {
		return 0;
	}
	try {
		Log::Internal::Push(FormatLuaArgs(L, 1), severity);
	} catch (const std::exception &e) {
		Log::Error("log format: {}", e.what());
	}
	return 0;
}

int LuaLogInfo(lua_State *L) {
	return LuaLog(L, Log::Severity::Info);
}

int LuaLogWarn(lua_State *L) {
	return LuaLog(L, Log::Severity::Warn);
}

int LuaLogError(lua_State *L) {
	return LuaLog(L, Log::Severity::Error);
}

} // namespace

std::string FormatLuaArgs(lua_State *L, int formatIndex) {
	const int count = lua_gettop(L);
	size_t formatLen = 0;
	const char *formatPtr = luaL_tolstring(L, formatIndex, &formatLen);
	const std::string format(formatPtr != nullptr ? formatPtr : "", formatLen);
	lua_pop(L, 1);

	fmt::dynamic_format_arg_store<fmt::format_context> store;
	for (int i = formatIndex + 1; i <= count; ++i) {
		switch (lua_type(L, i)) {
		case LUA_TBOOLEAN:
			store.push_back(lua_toboolean(L, i) != 0);
			break;
		case LUA_TNUMBER:
			if (lua_isinteger(L, i)) {
				store.push_back(lua_tointeger(L, i));
			} else {
				store.push_back(lua_tonumber(L, i));
			}
			break;
		case LUA_TNIL:
			store.push_back(std::string("nil"));
			break;
		default: {
			size_t len = 0;
			const char *text = luaL_tolstring(L, i, &len);
			if (text != nullptr) {
				store.push_back(std::string(text, len));
			} else {
				store.push_back(std::string("nil"));
			}
			lua_pop(L, 1);
			break;
		}
		}
	}

	return fmt::vformat(format, store);
}

void BindLog(sol::state &lua) {
	sol::table log = lua.create_named_table("log");
	log["info"] = &LuaLogInfo;
	log["warn"] = &LuaLogWarn;
	log["error"] = &LuaLogError;
}

namespace {

void TimerOnTimeout(Timer &timer, sol::object fn) {
	if (!fn.valid() || fn.get_type() == sol::type::lua_nil) {
		timer.ClearOnTimeout();
		return;
	}
	if (fn.get_type() != sol::type::function) {
		Log::Error("timer:on_timeout: expected a function");
		return;
	}
	sol::protected_function callback = fn;
	timer.SetOnTimeout([callback]() mutable {
		sol::protected_function_result result = callback();
		if (result.valid()) {
			return;
		}
		const sol::error err = result;
		Log::Error("Lua error in timer: {}", err.what());
	});
}

struct TimerHost {
	ScriptServer *scripts = nullptr;
};

Timer *TimerHostCreate(TimerHost &host, f32 timeout, sol::optional<bool> autoStart,
	sol::optional<bool> repeating) {
	if (host.scripts == nullptr) {
		return nullptr;
	}
	return host.scripts->NewTimer(timeout, autoStart.value_or(false), repeating.value_or(false));
}

void BindTimers(sol::state &lua, ScriptServer &scripts) {
	lua.new_usertype<Timer>("Timer", sol::no_constructor, "start", &Timer::Start, "pause",
		&Timer::Pause, "stop", &Timer::Stop, "on_timeout", &TimerOnTimeout, "is_running",
		&Timer::IsRunning, "is_paused", &Timer::IsPaused, "elapsed",
		sol::readonly_property(&Timer::Elapsed), "remaining",
		sol::readonly_property(&Timer::Remaining), "timeout",
		sol::property(&Timer::Timeout, &Timer::SetTimeout), "repeating",
		sol::property(&Timer::IsRepeating, &Timer::SetRepeating));
	lua.new_usertype<TimerHost>("Timers", sol::no_constructor, "create", &TimerHostCreate);
	lua["timers"] = TimerHost{&scripts};
}

void TweenOnUpdate(Tween &tween, sol::object fn) {
	if (!fn.valid() || fn.get_type() == sol::type::lua_nil) {
		tween.ClearOnUpdate();
		return;
	}
	if (fn.get_type() != sol::type::function) {
		Log::Error("tween:on_update: expected a function");
		return;
	}
	sol::protected_function callback = fn;
	tween.SetOnUpdate([callback](f32 value) mutable {
		sol::protected_function_result result = callback(value);
		if (result.valid()) {
			return;
		}
		const sol::error err = result;
		Log::Error("Lua error in tween: {}", err.what());
	});
}

void TweenOnFinish(Tween &tween, sol::object fn) {
	if (!fn.valid() || fn.get_type() == sol::type::lua_nil) {
		tween.ClearOnFinish();
		return;
	}
	if (fn.get_type() != sol::type::function) {
		Log::Error("tween:on_finish: expected a function");
		return;
	}
	sol::protected_function callback = fn;
	tween.SetOnFinish([callback]() mutable {
		sol::protected_function_result result = callback();
		if (result.valid()) {
			return;
		}
		const sol::error err = result;
		Log::Error("Lua error in tween: {}", err.what());
	});
}

struct TweenHost {
	ScriptServer *scripts = nullptr;
};

Tween *TweenHostCreate(TweenHost &host, f32 duration, sol::optional<bool> autoStart) {
	if (host.scripts == nullptr) {
		return nullptr;
	}
	return host.scripts->NewTween(duration, autoStart.value_or(true));
}

void TweenEase(Tween &tween, sol::object value) {
	if (!value.valid() || value.get_type() == sol::type::lua_nil) {
		tween.ClearEase();
		return;
	}
	if (!IsLuaNumber(value)) {
		Log::Error("tween:ease: expected an Easing or nil");
		return;
	}
	tween.SetEase(static_cast<Easing>(value.as<i32>()));
}

void BindTweens(sol::state &lua, ScriptServer &scripts) {
	lua.new_usertype<Tween>("Tween", sol::no_constructor, "start", &Tween::Start, "pause",
		&Tween::Pause, "stop", &Tween::Stop, "ease", &TweenEase, "on_update", &TweenOnUpdate,
		"on_finish", &TweenOnFinish, "is_running", &Tween::IsRunning, "is_paused", &Tween::IsPaused,
		"is_finished", &Tween::IsFinished, "elapsed", sol::readonly_property(&Tween::Elapsed),
		"remaining", sol::readonly_property(&Tween::Remaining), "progress",
		sol::readonly_property(&Tween::Progress), "duration",
		sol::property(&Tween::Duration, &Tween::SetDuration));
	lua.new_usertype<TweenHost>("Tweens", sol::no_constructor, "create", &TweenHostCreate);
	lua["tweens"] = TweenHost{&scripts};
}

void BindInput(sol::state &lua, Input &input) {
	lua.new_usertype<Input>(
		"Input", sol::no_constructor, "is_key_down",
		[](const Input &in, i32 key) { return in.IsKeyDown(static_cast<Key>(key)); },
		"is_key_just_pressed",
		[](const Input &in, i32 key) { return in.IsKeyJustPressed(static_cast<Key>(key)); },
		"is_key_just_released",
		[](const Input &in, i32 key) { return in.IsKeyJustReleased(static_cast<Key>(key)); },
		"mouse_position", &Input::GetMousePosition, "is_action_pressed",
		[](const Input &in, std::string_view name) { return in.IsActionPressed(name); },
		"is_action_just_pressed",
		[](const Input &in, std::string_view name) { return in.IsActionJustPressed(name); },
		"is_action_just_released",
		[](const Input &in, std::string_view name) { return in.IsActionJustReleased(name); },
		"get_action_strength",
		[](const Input &in, std::string_view name) { return in.GetActionStrength(name); },
		"get_axis",
		[](const Input &in, std::string_view negative, std::string_view positive) {
			return in.GetAxis(negative, positive);
		},
		"get_vector",
		[](const Input &in, std::string_view negativeX, std::string_view positiveX,
			std::string_view negativeY, std::string_view positiveY) {
			return in.GetVector(negativeX, positiveX, negativeY, positiveY);
		});
	lua["input"] = &input;
}

sol::object SceneRootLua(const Scene &scene, sol::this_state s) {
	if (!scene.IsValid()) {
		return Nil(s);
	}
	return PushEntity(s, scene.Root());
}

sol::object SceneActiveCamera(const Scene &scene, sol::this_state s) {
	if (!scene.IsValid()) {
		return Nil(s);
	}
	return PushEntity(s, scene.ActiveCamera());
}

void SceneSetActiveCamera(Scene &scene, sol::object value) {
	if (!scene.IsValid()) {
		return;
	}
	if (!value.valid() || value.get_type() == sol::type::lua_nil) {
		scene.SetActiveCamera(Entity());
		return;
	}
	if (value.is<Entity>()) {
		scene.SetActiveCamera(value.as<Entity>());
	} else {
		Log::Warn("Script: scene.active_camera expected Entity or nil");
	}
}

void BindScene(sol::state &lua) {
	lua.new_usertype<Scene>("Scene", sol::no_constructor, "is_valid", &Scene::IsValid, "name",
		sol::readonly_property(&Scene::Name), "root", &SceneRootLua, "active_camera",
		sol::property(&SceneActiveCamera, &SceneSetActiveCamera), sol::meta_function::to_string,
		[](const Scene &scene) {
			return fmt::format("Scene({})", scene.IsValid() ? scene.Name() : "<invalid>");
		});
}

sol::object ScenesCurrent(SceneManager &scenes, sol::this_state s) {
	Scene current = scenes.Current();
	if (!current.IsValid()) {
		return Nil(s);
	}
	return sol::make_object(s, current);
}

void BindScenes(sol::state &lua, SceneManager &scenes) {
	lua.new_usertype<SceneManager>("SceneManager", sol::no_constructor, "current", &ScenesCurrent,
		"change", &SceneManager::Change, "play_project", &SceneManager::PlayProject, "play_current",
		&SceneManager::PlayCurrent, "stop", &SceneManager::Stop, "is_playing",
		&SceneManager::IsPlaying);
	lua["scenes"] = &scenes;
}

// Use strings for 64-bit IDs; accept numeric IDs for compatibility.
std::string ResourceIdToLua(u64 id) {
	return std::to_string(id);
}

bool ResourceIdFromLua(const sol::object &value, u64 &out) {
	if (value.is<std::string>()) {
		const std::string text = value.as<std::string>();
		try {
			out = std::stoull(text);
			return true;
		} catch (const std::exception &) {
			return false;
		}
	}
	if (IsLuaNumber(value)) {
		const double number = value.as<double>();
		if (number < 0.0 || number >= 18446744073709551616.0) {
			return false;
		}
		out = static_cast<u64>(number);
		return true;
	}
	return false;
}

sol::object FindPackedScene(ResourceRegistry &resources, const std::string &path,
	sol::this_state s) {
	u64 foundId = 0;
	resources.Each([&](ResourceID id, Resource &resource, std::string_view typeName) {
		if (foundId != 0) {
			return;
		}
		if (typeName != "PackedScene") {
			return;
		}
		if (static_cast<PackedScene &>(resource).path == path) {
			foundId = id.value;
		}
	});
	if (foundId == 0) {
		return Nil(s);
	}
	return sol::make_object(s, ResourceIdToLua(foundId));
}

sol::object InstantiatePacked(ResourceRegistry &resources, World &world, sol::object id,
	sol::optional<Entity> parent, sol::this_state s) {
	u64 value = 0;
	if (!ResourceIdFromLua(id, value)) {
		return Nil(s);
	}
	Ref<PackedScene> packed = resources.Get<PackedScene>(ResourceID{value});
	if (packed == nullptr) {
		return Nil(s);
	}
	Entity spawned = packed->Instantiate(world, resources, parent.value_or(Entity()));
	return PushEntity(s, spawned);
}

sol::object ResourcesGet(ResourceRegistry &registry, sol::object id, sol::this_state s) {
	u64 value = 0;
	if (!ResourceIdFromLua(id, value)) {
		return Nil(s);
	}
	const ResourceID resourceId{value};
	if (!registry.Has(resourceId)) {
		return Nil(s);
	}
	return sol::make_object(s, ResourceView{&registry, resourceId});
}

void BindResources(sol::state &lua, World &world, ResourceRegistry &resources) {
	lua.new_usertype<ResourceRegistry>("ResourceRegistry", sol::no_constructor, "get",
		&ResourcesGet, "find_packed_scene", &FindPackedScene, "instantiate",
		[&world](ResourceRegistry &registry, sol::object id, sol::optional<Entity> parent,
			sol::this_state s) { return InstantiatePacked(registry, world, id, parent, s); });
	lua["resources"] = &resources;
}

void BindSettings(sol::state &lua, const ProjectSettings &project) {
	sol::table settings = lua.create_named_table("settings");
	settings["main_scene"] = project.application.mainScene;
	settings["viewport_width"] = project.rendering.viewport.width;
	settings["viewport_height"] = project.rendering.viewport.height;
}

void BindGlobalStore(sol::state &lua, StringStore *store) {
	if (store == nullptr) {
		return;
	}
	lua.new_usertype<StringStore>("StringStore", sol::no_constructor, "clear", &StringStore::Clear,
		"set", &StringStore::Set, "get", &StringStore::Get, "has", &StringStore::Has, "remove",
		&StringStore::Remove);
	lua["global_store"] = store;
}

} // namespace

void BindScriptApi(sol::state &lua, World &world, ScriptServer &scripts) {
	lua.registry()["sowa_scripts"] = static_cast<void *>(&scripts);
	BindVector2(lua);
	BindRect(lua);
	BindVector3(lua);
	BindColor(lua);
	BindMath(lua);
	BindEasing(lua);
	BindFieldProxy(lua);
	BindComponentView(lua);
	BindEntity(lua);
	BindLuaSystemBuilder(lua);
	BindWorld(lua, world, scripts);
	BindTimers(lua, scripts);
	BindTweens(lua, scripts);
}

void BindEngineApi(sol::state &lua, World &world, Input &input, SceneManager &scenes,
	ResourceRegistry &resources, const ProjectSettings &settings, AudioMixer *mixer,
	PhysicsWorld *physics, StringStore *globalStore) {
	if (mixer != nullptr) {
		lua.registry()["sowa_audio_mixer"] = static_cast<void *>(mixer);
	} else {
		lua.registry()["sowa_audio_mixer"] = sol::lua_nil;
	}
	if (physics != nullptr) {
		lua.registry()["sowa_physics"] = static_cast<void *>(physics);
	} else {
		lua.registry()["sowa_physics"] = sol::lua_nil;
	}
	BindPhysics(lua, physics);
	auto particles = lua.create_named_table("particles");
	particles.set_function("restart", [](Entity entity) {
		auto *emitter = entity.Get<Particles2D>();
		if (!emitter) {
			return false;
		}
		emitter->isEmitting = true;
		emitter->Restart();
		return true;
	});
	particles.set_function("set_emitting", [](Entity entity, bool enabled) {
		auto *emitter = entity.Get<Particles2D>();
		if (!emitter) {
			return false;
		}
		emitter->SetEmitting(enabled);
		return true;
	});
	particles.set_function("is_emitting", [](Entity entity) {
		const auto *emitter = entity.Get<Particles2D>();
		return emitter && emitter->IsEmitting();
	});
	particles.set_function("is_finished", [](Entity entity) {
		const auto *emitter = entity.Get<Particles2D>();
		return emitter && emitter->IsFinished();
	});
	particles.set_function("count", [](Entity entity) {
		const auto *emitter = entity.Get<Particles2D>();
		return emitter ? emitter->runtime.particles.size() : size_t{0};
	});
	auto animatedSprite = lua.create_named_table("animated_sprite");
	animatedSprite.set_function("play",
		[&resources](Entity entity, sol::optional<std::string> name) {
			return PlayAnimatedSprite(entity, resources, name.value_or(""));
		});
	animatedSprite.set_function("stop", &StopAnimatedSprite);
	animatedSprite.set_function("pause", [](Entity entity, sol::optional<bool> paused) {
		if (auto *sprite = entity.Get<AnimatedSprite>()) {
			// Pausing before the first update must suppress autoplay as well.
			sprite->playback.initialized = true;
			sprite->isPlaying = !paused.value_or(true);
		}
	});
	animatedSprite.set_function("is_playing", [](Entity entity) {
		const auto *sprite = entity.Get<AnimatedSprite>();
		return sprite && sprite->isPlaying;
	});
	animatedSprite.set_function("frame", [](Entity entity) {
		const auto *sprite = entity.Get<AnimatedSprite>();
		return sprite ? sprite->frame : 0;
	});

	auto animation = lua.create_named_table("animation");
	animation.set_function("play",
		[&world, &resources](Entity entity, sol::optional<std::string> name) {
			return PlayAnimation(world, resources, entity, name.value_or(""));
		});
	animation.set_function("seek", [&world, &resources](Entity entity, f64 time) {
		return SeekAnimation(world, resources, entity, time);
	});
	animation.set_function("pause", [](Entity entity, sol::optional<bool> paused) {
		PauseAnimation(entity, paused.value_or(true));
	});
	animation.set_function("stop", &StopAnimation);
	animation.set_function("time", [](Entity entity) {
		const auto *player = entity.Get<AnimationPlayer>();
		return player ? player->playback.time : 0.;
	});
	animation.set_function("is_playing", [](Entity entity) {
		const auto *player = entity.Get<AnimationPlayer>();
		return player && player->isPlaying;
	});
	BindKeyTable(lua);
	BindLog(lua);
	BindInput(lua, input);
	BindScene(lua);
	BindScenes(lua, scenes);
	BindResourceFieldProxy(lua);
	BindResourceView(lua);
	BindResources(lua, world, resources);
	BindSettings(lua, settings);
	BindGlobalStore(lua, globalStore);
}

} // namespace sowa
