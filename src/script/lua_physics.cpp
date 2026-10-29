#include "script/lua_physics.hpp"
#include "script/lua_function.hpp"

#include <cmath>
#include <stdexcept>

#include "physics/physics_world.hpp"

namespace sowa {
namespace {
[[noreturn]] void Error(const std::string &message) {
	throw std::invalid_argument("physics: " + message);
}

Vector2 Vector(sol::object value, const char *name) {
	if (value.is<Vector2>()) {
		return value.as<Vector2>();
	}
	if (value.is<FieldProxy>()) {
		auto proxy = value.as<FieldProxy>();
		if (const auto *vector = proxy.entity.GetField(proxy.type, proxy.fieldName).As<Vector2>()) {
			return *vector;
		}
	}
	Error(std::string(name) + " must be a Vector2");
}

u32 Mask(sol::object value, const char *name) {
	if (value.get_type() != sol::type::number) {
		Error(std::string(name) + " must be an unsigned 32-bit integer");
	}
	const double number = value.as<double>();
	if (!std::isfinite(number) || number < 0 || number > 4294967295.0 ||
		std::floor(number) != number) {
		Error(std::string(name) + " must be an unsigned 32-bit integer");
	}
	return static_cast<u32>(number);
}

std::vector<Entity> Entities(sol::object value, const char *name) {
	if (value.get_type() != sol::type::table) {
		Error(std::string(name) + " must be an array of Entity handles");
	}
	std::vector<Entity> result;
	auto table = value.as<sol::table>();
	for (const auto &entry : table) {
		if (entry.first.get_type() != sol::type::number || !entry.second.is<Entity>()) {
			Error(std::string(name) + " must be an array of Entity handles");
		}
		const double index = entry.first.as<double>();
		if (!std::isfinite(index) || std::floor(index) != index || index < 1 ||
			index > table.size()) {
			Error(std::string(name) + " must be an array of Entity handles");
		}
		result.push_back(entry.second.as<Entity>());
	}
	if (result.size() != table.size()) {
		Error(std::string(name) + " must be a dense array of Entity handles");
	}
	return result;
}

PhysicsQueryOptions2D Options(sol::optional<sol::object> object) {
	PhysicsQueryOptions2D options;
	if (!object) {
		return options;
	}
	if (object->get_type() != sol::type::table) {
		Error("query options must be a table or nil");
	}
	const auto table = object->as<sol::table>();
	for (const auto &entry : table) {
		if (!entry.first.is<std::string>()) {
			Error("query option keys must be strings");
		}
		const auto key = entry.first.as<std::string>();
		if (key == "collisionLayer") {
			options.collisionLayer = Mask(entry.second, "collisionLayer");
		} else if (key == "collisionMask") {
			options.collisionMask = Mask(entry.second, "collisionMask");
		} else if (key == "includeSensors") {
			if (entry.second.get_type() != sol::type::boolean) {
				Error("includeSensors must be a boolean");
			}
			options.includeSensors = entry.second.as<bool>();
		} else if (key == "excludeBodies") {
			options.excludeBodies = Entities(entry.second, "excludeBodies");
		} else if (key == "excludeColliders") {
			options.excludeColliders = Entities(entry.second, "excludeColliders");
		} else {
			Error("unknown query option '" + key + "'");
		}
	}
	return options;
}

PhysicsQueryShape2D Shape(sol::table table) {
	for (const auto &entry : table) {
		if (!entry.first.is<std::string>()) {
			Error("shape descriptor keys must be strings");
		}
		const auto key = entry.first.as<std::string>();
		if (key != "type" && key != "position" && key != "rotation" && key != "size" &&
			key != "radius") {
			Error("unknown shape field '" + key + "'");
		}
	}
	PhysicsQueryShape2D shape;
	sol::object kind = table["type"];
	if (!kind.is<std::string>()) {
		Error("shape.type must be Box, Circle or Capsule");
	}
	const auto name = kind.as<std::string>();
	if (name == "Box") {
		shape.type = ShapeType2D::Box;
	} else if (name == "Circle") {
		shape.type = ShapeType2D::Circle;
	} else if (name == "Capsule") {
		shape.type = ShapeType2D::Capsule;
	} else {
		Error("shape.type must be Box, Circle or Capsule");
	}
	shape.position = Vector(table["position"], "shape.position");
	if (shape.type != ShapeType2D::Circle) {
		shape.size = Vector(table["size"], "shape.size");
	}
	if (shape.type != ShapeType2D::Box) {
		sol::object radius = table["radius"];
		if (radius.get_type() != sol::type::number) {
			Error("shape.radius must be a number");
		}
		shape.radius = radius.as<f32>();
	}
	sol::object rotation = table["rotation"];
	if (rotation.get_type() != sol::type::lua_nil) {
		if (rotation.get_type() != sol::type::number) {
			Error("shape.rotation must be a number");
		}
		shape.rotation = rotation.as<f32>();
	}
	return shape;
}

sol::table Overlap(sol::state_view lua, const PhysicsOverlap2D &hit) {
	auto table = lua.create_table();
	table["collider"] = hit.collider;
	table["body"] = hit.body;
	table["isSensor"] = hit.isSensor;
	return table;
}

sol::object Hit(sol::state_view lua, const std::optional<PhysicsHit2D> &hit) {
	if (!hit) {
		return sol::make_object(lua, sol::lua_nil);
	}
	auto table = Overlap(lua, *hit);
	table["point"] = hit->point;
	table["normal"] = hit->normal;
	table["fraction"] = hit->fraction;
	table["distance"] = hit->distance;
	return sol::make_object(lua, table);
}

sol::object Hit(sol::state_view lua, PhysicsClosestResult2D result) {
	if (!result) {
		Error(result.error);
	}
	return Hit(lua, result.value);
}

sol::table Hits(sol::state_view lua, PhysicsHitsResult2D result) {
	if (!result) {
		Error(result.error);
	}
	auto array = lua.create_table();
	int i = 1;
	for (const auto &hit : result.value) {
		array[i++] = Hit(lua, std::optional<PhysicsHit2D>{hit});
	}
	return array;
}

template <typename F> auto QueryService(PhysicsWorld *physics, F &&query) {
	if (physics != nullptr) {
		return query(*physics);
	}
	// Validate even without a bound service; valid queries return empty results.
	PhysicsWorld unavailable;
	return query(unavailable);
}

PhysicsWorld *Physics(sol::this_state state) {
	sol::state_view lua(state);
	sol::object object = lua.registry()["sowa_physics"];
	return object.is<void *>() ? static_cast<PhysicsWorld *>(object.as<void *>()) : nullptr;
}
} // namespace

void BindPhysics(sol::state &lua, PhysicsWorld *physics) {
	auto table = lua.create_named_table("physics");
	table["ALL_LAYERS"] = AllPhysicsLayers;
	table.set_function("layers", [](sol::variadic_args args) {
		u32 mask = 0;
		for (auto value : args) {
			u32 layer = Mask(sol::object(value), "layer number");
			if (layer < 1 || layer > 32) {
				Error("layer number must be between 1 and 32");
			}
			mask |= u32{1} << (layer - 1);
		}
		return mask;
	});
	table.set_function("raycast", [physics](sol::object from, sol::object to,
									  sol::optional<sol::object> options, sol::this_state state) {
		auto a = Vector(from, "from"), b = Vector(to, "to");
		auto opts = Options(options);
		return Hit(sol::state_view(state), QueryService(physics, [&](PhysicsWorld &service) {
			return service.RaycastClosest(a, b - a, opts);
		}));
	});
	table.set_function("raycast_all",
		[physics](sol::object from, sol::object to, sol::optional<sol::object> options,
			sol::this_state state) {
			auto a = Vector(from, "from"), b = Vector(to, "to");
			auto opts = Options(options);
			return Hits(sol::state_view(state), QueryService(physics, [&](PhysicsWorld &service) {
				return service.RaycastAll(a, b - a, opts);
			}));
		});
	table.set_function("shape_cast",
		[physics](sol::table descriptor, sol::object translation,
			sol::optional<sol::object> options, sol::this_state state) {
			auto shape = Shape(descriptor);
			auto movement = Vector(translation, "translation");
			auto opts = Options(options);
			return Hit(sol::state_view(state), QueryService(physics, [&](PhysicsWorld &service) {
				return service.ShapeCastClosest(shape, movement, opts);
			}));
		});
	table.set_function("shape_cast_all",
		[physics](sol::table descriptor, sol::object translation,
			sol::optional<sol::object> options, sol::this_state state) {
			auto shape = Shape(descriptor);
			auto movement = Vector(translation, "translation");
			auto opts = Options(options);
			return Hits(sol::state_view(state), QueryService(physics, [&](PhysicsWorld &service) {
				return service.ShapeCastAll(shape, movement, opts);
			}));
		});
	table.set_function("overlap", [physics](sol::table descriptor,
									  sol::optional<sol::object> options, sol::this_state state) {
		auto shape = Shape(descriptor);
		auto opts = Options(options);
		auto result = QueryService(physics,
			[&](PhysicsWorld &service) { return service.OverlapShape(shape, opts); });
		if (!result) {
			Error(result.error);
		}
		sol::state_view lua(state);
		auto array = lua.create_table();
		int i = 1;
		for (const auto &hit : result.value) {
			array[i++] = Overlap(lua, hit);
		}
		return array;
	});
}

sol::object RaycastMethod(ComponentView &view, const std::string &name, sol::this_state state) {
	sol::state_view lua(state);
	if (view.type.Name() != "Raycast2D") {
		return sol::make_object(lua, sol::lua_nil);
	}
	PhysicsWorld *physics = Physics(state);
	if (name == "hit") {
		return MakeLuaFunction(lua, [physics](ComponentView self, sol::this_state state) {
			return Hit(sol::state_view(state),
				physics ? physics->RaycastHit(self.entity) : std::optional<PhysicsHit2D>{});
		});
	}
	if (name == "is_colliding") {
		return MakeLuaFunction(lua, [physics](ComponentView self) {
			return physics && physics->RaycastHit(self.entity).has_value();
		});
	}
	if (name == "force_update") {
		return MakeLuaFunction(lua, [physics](ComponentView self, sol::this_state state) {
			return Hit(sol::state_view(state),
				physics ? physics->ForceRaycastUpdate(self.entity) : PhysicsClosestResult2D{});
		});
	}
	if (name == "add_body_exception" || name == "add_collider_exception") {
		const bool body = name == "add_body_exception";
		return MakeLuaFunction(lua, [physics, body](ComponentView self, Entity excluded) {
			if (physics && !physics->AddRaycastException(self.entity, excluded, body)) {
				Error("exception requires a live ray and Entity in the same world");
			}
		});
	}
	if (name == "remove_body_exception" || name == "remove_collider_exception") {
		const bool body = name == "remove_body_exception";
		return MakeLuaFunction(lua, [physics, body](ComponentView self, Entity excluded) {
			if (physics) {
				physics->RemoveRaycastException(self.entity, excluded, body);
			}
		});
	}
	if (name == "clear_exceptions") {
		return MakeLuaFunction(lua, [physics](ComponentView self) {
			if (physics) {
				physics->ClearRaycastExceptions(self.entity);
			}
		});
	}
	return sol::make_object(lua, sol::lua_nil);
}

void ValidatePhysicsField(const ComponentView &view, const std::string &name, sol::object value) {
	if ((view.type.Name() == "Collider2D" || view.type.Name() == "Raycast2D") &&
		(name == "collisionLayer" || name == "collisionMask")) {
		Mask(value, name.c_str());
	}
}
} // namespace sowa
