#include "core/project_settings.hpp"

#include <algorithm>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "core/input_binding.hpp"
#include "core/log.hpp"
#include "core/serialize/archive.hpp"

namespace sowa {

namespace {

constexpr std::string_view kProjectFile = "res://project.sowa";

} // namespace

// Input:
//   Actions:
//     jump: [Space, W]
//     quick_save: [Ctrl+S]
void ProjectSettings::serializeInput(Archive &archive, const std::vector<std::string> &rootKeys) {
	if (archive.IsSaving()) {
		Archive actionsNode = archive.Child("Input").Child("Actions");
		for (InputAction &action : input.actions) {
			std::vector<std::string> bindings;
			bindings.reserve(action.bindings.size());
			for (const InputBinding &binding : action.bindings) {
				bindings.push_back(FormatInputBinding(binding));
			}
			actionsNode.List(action.name, bindings,
				[](Archive &element, std::string &text) { element.Value(text); });
		}
		return;
	}

	if (std::find(rootKeys.begin(), rootKeys.end(), "Input") == rootKeys.end()) {
		return;
	}
	Archive actionsNode = archive.Child("Input").Child("Actions");
	std::vector<InputAction> loaded;
	for (const std::string &name : actionsNode.Keys()) {
		const bool duplicate = std::any_of(loaded.begin(), loaded.end(),
			[&](const InputAction &action) { return action.name == name; });
		if (duplicate) {
			actionsNode.AddError(name, "duplicate action");
			continue;
		}
		std::vector<std::string> texts;
		actionsNode.List(name, texts,
			[](Archive &element, std::string &text) { element.Value(text); });

		// A bad binding is dropped on its own; the rest of the action loads.
		InputAction action{name, {}};
		for (const std::string &text : texts) {
			if (const std::optional<InputBinding> binding = ParseInputBinding(text)) {
				action.bindings.push_back(*binding);
			} else {
				actionsNode.AddError(name, "invalid binding '" + text + "'");
			}
		}
		loaded.push_back(std::move(action));
	}
	input.actions = std::move(loaded);
}

void ProjectSettings::Serialize(Archive &archive) {
	Archive applicationNode = archive.Child("Application");
	applicationNode.Field("Name", application.name);
	applicationNode.Field("Author", application.author);
	applicationNode.Field("Version", application.version);
	applicationNode.Field("Icon", application.icon);
	applicationNode.Field("MainScene", application.mainScene);
	applicationNode.List("Scripts", application.scripts,
		[](Archive &element, std::string &path) { element.Value(path); });
	applicationNode.List("Autoloads", application.autoloads,
		[](Archive &element, std::string &path) { element.Value(path); });

	Archive renderingNode = archive.Child("Rendering");

	Archive windowNode = renderingNode.Child("Window");
	windowNode.Field("Width", rendering.window.width);
	windowNode.Field("Height", rendering.window.height);

	Archive viewportNode = renderingNode.Child("Viewport");
	viewportNode.Field("Width", rendering.viewport.width);
	viewportNode.Field("Height", rendering.viewport.height);
	viewportNode.EnumField("ScaleMode", rendering.viewport.scaleMode, ViewportScaleModeConstants());

	Archive physicsNode = archive.Child("Physics");
	i32 ticksPerSecond = physics.ticksPerSecond;
	physicsNode.Field("TicksPerSecond", ticksPerSecond);
	if (ticksPerSecond > 0) {
		physics.ticksPerSecond = ticksPerSecond;
	} else {
		physicsNode.AddError("TicksPerSecond", "must be greater than zero");
	}

	const std::vector<std::string> rootKeys = archive.Keys();
	serializeInput(archive, rootKeys);

	const bool hasStore = archive.IsSaving() || std::find(rootKeys.begin(), rootKeys.end(),
													"GlobalStore") != rootKeys.end();
	Archive storeNode = archive.Child("GlobalStore");
	if (archive.IsSaving()) {
		std::vector<std::string> keys;
		keys.reserve(globalStore.GetStore().size());
		for (const auto &[key, value] : globalStore.GetStore()) {
			(void)value;
			keys.push_back(key);
		}
		std::sort(keys.begin(), keys.end());
		for (const std::string &key : keys) {
			std::string value = globalStore.Get(key);
			storeNode.Field(key, value);
		}
	} else if (hasStore) {
		StringStore loaded;
		for (const std::string &key : storeNode.Keys()) {
			std::string value;
			if (storeNode.Field(key, value)) {
				loaded.Set(key, value);
			}
		}
		globalStore = std::move(loaded);
	}
}

bool ProjectSettings::Load(FileSystem &fs) {
	const ArchiveFileResult result = LoadFromFile(fs, kProjectFile, *this);

	// A bad file is fatal; a bad *field* is not. Absent keys aren't reported
	// at all - the members above already hold the defaults.
	if (!result) {
		if (!result.parsed) {
			for (const std::string &problem : result.problems) {
				Log::Error("{}: {}", kProjectFile, problem);
			}
		} else {
			Log::Error("Failed to load {}: {}", kProjectFile, FsErrorToString(result.error));
		}
		return false;
	}

	for (const std::string &problem : result.problems) {
		Log::Warn("{}: {}", kProjectFile, problem);
	}

	return true;
}

bool ProjectSettings::Save(FileSystem &fs) {
	const ArchiveFileResult result = SaveToFile(fs, kProjectFile, *this);
	if (!result) {
		Log::Error("Failed to save {}: {}", kProjectFile, FsErrorToString(result.error));
		return false;
	}
	return true;
}

} // namespace sowa
