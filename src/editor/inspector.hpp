#pragma once

#include <string>
#include <string_view>
#include <vector>

#include "ecs/component_type.hpp"
#include "ecs/entity.hpp"

namespace sowa {
class FileSystem;
class ProjectSettings;
class SceneManager;
class ScriptServer;
class World;
} // namespace sowa

namespace sowa::editor {

// Allows edits when the entity is alive, the session is stopped, and the row is not
// instance-locked. Viewing remains allowed.
bool CanEdit(Entity entity, bool playing);
// Allows field and name edits through CanEdit while stopped. During Play, only runtime
// scene entities are editable; frozen editor documents remain read-only. Live edits bypass
// history and are discarded on Stop. Structural edits still require CanEdit.
bool CanEditFields(Entity entity, const SceneManager &scenes);
bool CanRemoveComponent(Entity entity, ComponentType type, bool playing = false);
std::string_view ComponentCategory(ComponentType type);
std::vector<ComponentType> AddableComponents(const World &world, Entity entity);

// Turns a reflected identifier into a panel label: first word capital,
// later words lower, with a space at each camelCase break.
// "degreesPerSecond" -> "Degrees per second".
std::string DisplayName(std::string_view ident);

// Fields with FieldFlags::Expose, in registration order.
std::vector<const FieldInfo *> ExposedFields(ComponentType type);

struct InspectedField {
	ComponentType component;
	const FieldInfo *field = nullptr;
};

// Every exposed field on the entity, grouped by GetComponents() order.
std::vector<InspectedField> CollectFields(Entity entity);

struct ScriptEditResult {
	bool success = false;
	std::string path;
	std::string error;

	explicit operator bool() const {
		return success;
	}
};

// Loads a .lua component definition when needed, attaches all component
// types declared by that file, and makes a newly loaded definition a project
// startup script so future scene loads know its component schema.
ScriptEditResult TryAttachEntityScript(ScriptServer &scripts, FileSystem &fs,
	ProjectSettings &project, Entity entity, std::string_view path, bool playing);

// Removes all component types declared by one attached script. The project
// keeps loading its definition because other entities/scenes may still use it.
bool TryRemoveEntityScript(ScriptServer &scripts, Entity entity, std::string_view path,
	bool playing);

} // namespace sowa::editor
