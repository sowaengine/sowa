#include "animation/animation.hpp"

#include <cstring>

#include "ecs/world.hpp"

namespace sowa {
Entity ResolveAnimationTarget(Entity root, std::string_view path) {
	while (!path.empty() && root.IsAlive()) {
		const auto slash = path.find('/');
		const auto part = path.substr(0, slash);
		if (part == "..") {
			root = root.GetParent();
		} else if (!part.empty() && part != ".") {
			root = root.Find(part);
		}
		if (slash == std::string_view::npos) {
			break;
		}
		path.remove_prefix(slash + 1);
	}
	return root;
}

std::optional<AnimationValue> ReadAnimationProperty(World &world, Entity root,
	const AnimationBinding &binding) {
	Entity target = ResolveAnimationTarget(root, binding.path);
	if (!target.IsAlive()) {
		return {};
	}
	if (binding.component.empty() && binding.property == "isVisible" &&
		binding.type == FieldType::Bool) {
		return target.IsVisible();
	}
	auto field = target.GetField(world.FindComponent(binding.component), binding.property);
	if (!field || field.Type() != binding.type || !field.Info()->IsExposed()) {
		return {};
	}
	if (binding.type == FieldType::EntityRef) {
		Entity value = *field.As<Entity>();
		// Paths from the common tree root remain portable across scene instances.
		std::string prefix;
		Entity base = root;
		while (base.HasParent()) {
			base = base.GetParent();
			prefix += "../";
		}
		if (!value.IsAlive()) {
			return std::string();
		}
		std::vector<std::string> parts;
		for (Entity e = value; e != base; e = e.GetParent()) {
			if (!e.IsAlive()) {
				return {};
			}
			parts.push_back(e.GetName());
		}
		for (auto it = parts.rbegin(); it != parts.rend(); ++it) {
			prefix += *it;
			prefix += '/';
		}
		return prefix.empty() ? std::string(".") : prefix;
	}
	AnimationValue result = DefaultAnimationValue(binding.type);
	std::visit(
		[&](auto &value) {
			using T = std::decay_t<decltype(value)>;
			if constexpr (std::is_same_v<T, i32>) {
				if (binding.type == FieldType::Enum) {
					std::memcpy(&value, field.Data(), sizeof(value));
				} else {
					field.Get(value);
				}
			} else {
				field.Get(value);
			}
		},
		result);
	return result;
}

bool WriteAnimationProperty(World &world, Entity root, const AnimationBinding &binding,
	const AnimationValue &value) {
	if (!AnimationValueMatches(binding.type, value)) {
		return false;
	}
	Entity target = ResolveAnimationTarget(root, binding.path);
	if (!target.IsAlive()) {
		return false;
	}
	if (binding.component.empty() && binding.property == "isVisible" &&
		binding.type == FieldType::Bool) {
		target.SetVisible(std::get<bool>(value));
		return true;
	}
	auto field = target.GetField(world.FindComponent(binding.component), binding.property);
	if (!field || field.Type() != binding.type || !field.Info()->IsExposed()) {
		return false;
	}
	if (binding.type == FieldType::EntityRef) {
		auto &path = std::get<std::string>(value);
		return field.Set(path.empty() ? Entity() : ResolveAnimationTarget(root, path));
	}
	return std::visit(
		[&](const auto &v) {
			using T = std::decay_t<decltype(v)>;
			if constexpr (std::is_same_v<T, i32>) {
				if (binding.type == FieldType::Enum) {
					std::memcpy(field.Data(), &v, sizeof(v));
					return true;
				}
			}
			return field.Set(v);
		},
		value);
}
} // namespace sowa
