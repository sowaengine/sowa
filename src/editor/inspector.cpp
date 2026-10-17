#include "editor/inspector.hpp"
#include "ecs/world.hpp"

#include <algorithm>
#include <cctype>

#include "core/filesystem/file_system.hpp"
#include "core/project_settings.hpp"
#include "editor/filesystem_browser.hpp"
#include "editor/gizmos.hpp"
#include "scene/scene.hpp"
#include "scene/scene_manager.hpp"
#include "script/script_server.hpp"

namespace sowa::editor {

std::string DisplayName(std::string_view ident) {
	std::string out;
	out.reserve(ident.size() + 8);
	for (size_t i = 0; i < ident.size(); ++i) {
		const unsigned char c = static_cast<unsigned char>(ident[i]);
		if (c == '_' || c == '-') {
			if (!out.empty() && out.back() != ' ') {
				out += ' ';
			}
			continue;
		}
		if (std::isupper(c) && !out.empty()) {
			const unsigned char prev = static_cast<unsigned char>(ident[i - 1]);
			const bool split = std::islower(prev) || std::isdigit(prev) ||
							   (i + 1 < ident.size() && std::isupper(prev) &&
								   std::islower(static_cast<unsigned char>(ident[i + 1])));
			if (split) {
				out += ' ';
			}
		}
		if (out.empty() || out.back() == ' ') {
			out += static_cast<char>(out.empty() ? std::toupper(c) : std::tolower(c));
		} else {
			out += static_cast<char>(std::tolower(c));
		}
	}
	return out;
}

std::vector<ComponentType> AddableComponents(const World &world, Entity entity) {
	std::vector<ComponentType> types;
	for (ComponentType type : world.Components()) {
		if (!entity.Has(type) && type.Name() != "SceneRoot" && type.Name() != "Hidden") {
			types.push_back(type);
		}
	}
	return types;
}

bool CanRemoveComponent(Entity entity, ComponentType type, bool playing) {
	return CanEdit(entity, playing) && type.IsValid() && entity.Has(type) &&
		   type.Name() != "SceneRoot" && type.Name() != "Hidden";
}

std::string_view ComponentCategory(ComponentType type) {
	const auto name = type.Name();
	if (name.starts_with("UI")) {
		return "User Interface";
	}
	if (name == "RigidBody2D" || name == "Collider2D" || name == "Raycast2D") {
		return "Physics";
	}
	if (name == "AudioSource" || name.starts_with("Audio")) {
		return "Audio";
	}
	if (name == "AnimationPlayer" || name == "AnimatedSprite") {
		return "Animation";
	}
	if (name == "Sprite" || name == "Text2D" || name == "NinePatchRect" || name == "Tilemap" ||
		name == "Particles2D" || name == "Camera2D" || name == "Parallax2D" ||
		name == "RenderOrder") {
		return "Rendering";
	}
	if (name == "Transform2D") {
		return "General";
	}
	return "Gameplay and Scripts";
}

bool CanEdit(Entity entity, bool playing) {
	if (!entity.IsAlive()) {
		return false;
	}
	if (playing) {
		return false;
	}
	if (IsInstanceLocked(entity)) {
		return false;
	}
	return true;
}

bool CanEditFields(Entity entity, const SceneManager &scenes) {
	if (!scenes.IsPlaying()) {
		return CanEdit(entity, false);
	}
	if (!entity.IsAlive()) {
		return false;
	}
	for (const Scene scene : scenes.RuntimeScenes()) {
		if (IsInSceneTree(entity, scene.Root())) {
			return true;
		}
	}
	return false;
}

std::vector<const FieldInfo *> ExposedFields(ComponentType type) {
	std::vector<const FieldInfo *> fields;
	if (!type.IsValid()) {
		return fields;
	}
	for (const FieldInfo &field : type.Fields()) {
		if (field.IsExposed()) {
			fields.push_back(&field);
		}
	}
	return fields;
}

std::vector<InspectedField> CollectFields(Entity entity) {
	std::vector<InspectedField> out;
	if (!entity.IsAlive()) {
		return out;
	}
	for (ComponentType type : entity.GetComponents()) {
		for (const FieldInfo *field : ExposedFields(type)) {
			out.push_back({type, field});
		}
	}
	return out;
}

ScriptEditResult TryAttachEntityScript(ScriptServer &scripts, FileSystem &fs,
	ProjectSettings &project, Entity entity, std::string_view path, bool playing) {
	ScriptEditResult result;
	if (!CanEdit(entity, playing)) {
		result.error = "This entity cannot be edited.";
		return result;
	}
	if (KindFromName(path, false) != AssetKind::Script) {
		result.error = "Choose a .lua file.";
		return result;
	}

	const FsResult<PathData> resolved = fs.Resolve(path);
	if (!resolved) {
		result.error = "The script path is invalid.";
		return result;
	}
	result.path = resolved.value.scheme + "://" + resolved.value.path;

	if (scripts.ScriptComponentTypes(result.path).empty() && !scripts.DoFile(fs, result.path)) {
		result.error = "The script could not be loaded. See Console for details.";
		return result;
	}
	if (scripts.ScriptComponentTypes(result.path).empty()) {
		result.error = "This file does not define a script component.";
		return result;
	}
	if (!scripts.AttachScript(entity, result.path)) {
		result.error = "The script could not be attached.";
		return result;
	}

	if (!scripts.IsProjectScriptSource(result.path) &&
		std::find(project.application.scripts.begin(), project.application.scripts.end(),
			result.path) == project.application.scripts.end()) {
		project.application.scripts.push_back(result.path);
	}
	result.success = true;
	return result;
}

bool TryRemoveEntityScript(ScriptServer &scripts, Entity entity, std::string_view path,
	bool playing) {
	return CanEdit(entity, playing) && scripts.RemoveScript(entity, path);
}

} // namespace sowa::editor
