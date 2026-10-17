#include "editor/project_settings.hpp"

#include <algorithm>
#include <string_view>
#include <unordered_set>


namespace sowa::editor {
namespace {

std::string validatePaths(const std::vector<std::string> &paths, std::string_view label) {
	std::unordered_set<std::string> seen;
	for (const std::string &path : paths) {
		if (path.empty()) {
			return std::string(label) + " paths cannot be empty.";
		}
		if (!seen.insert(path).second) {
			return std::string(label) + " contains the duplicate path '" + path + "'.";
		}
	}
	return {};
}

std::string validateActions(const std::vector<InputAction> &actions) {
	std::unordered_set<std::string> names;
	for (const InputAction &action : actions) {
		if (action.name.empty()) {
			return "Input action names cannot be empty.";
		}
		if (!names.insert(action.name).second) {
			return "Input contains the duplicate action '" + action.name + "'.";
		}
	}
	return {};
}

} // namespace

bool EditableProjectSettingsEqual(const ProjectSettings &a, const ProjectSettings &b) {
	return a.application.name == b.application.name &&
		   a.application.author == b.application.author &&
		   a.application.version == b.application.version &&
		   a.application.icon == b.application.icon &&
		   a.application.mainScene == b.application.mainScene &&
		   a.application.scripts == b.application.scripts &&
		   a.application.autoloads == b.application.autoloads &&
		   a.rendering.window.width == b.rendering.window.width &&
		   a.rendering.window.height == b.rendering.window.height &&
		   a.rendering.viewport.width == b.rendering.viewport.width &&
		   a.rendering.viewport.height == b.rendering.viewport.height &&
		   a.rendering.viewport.scaleMode == b.rendering.viewport.scaleMode &&
		   a.input.actions == b.input.actions &&
		   a.globalStore.GetStore() == b.globalStore.GetStore();
}

std::string ValidateEditableProjectSettings(const ProjectSettings &settings) {
	if (settings.rendering.window.width <= 0 || settings.rendering.window.height <= 0) {
		return "Window width and height must be greater than zero.";
	}
	if (settings.rendering.viewport.width <= 0 || settings.rendering.viewport.height <= 0) {
		return "Viewport width and height must be greater than zero.";
	}
	if (std::string problem = validatePaths(settings.application.scripts, "Scripts");
		!problem.empty()) {
		return problem;
	}
	if (std::string problem = validatePaths(settings.application.autoloads, "Autoloads");
		!problem.empty()) {
		return problem;
	}
	return validateActions(settings.input.actions);
}

std::vector<std::string> ActionsSharingBinding(const std::vector<InputAction> &actions,
	std::size_t actionIndex, const InputBinding &binding) {
	std::vector<std::string> names;
	for (std::size_t i = 0; i < actions.size(); ++i) {
		const std::vector<InputBinding> &bindings = actions[i].bindings;
		if (i != actionIndex &&
			std::find(bindings.begin(), bindings.end(), binding) != bindings.end()) {
			names.push_back(actions[i].name);
		}
	}
	return names;
}

std::string UniqueActionName(const std::vector<InputAction> &actions, const std::string &base) {
	const auto taken = [&](const std::string &name) {
		return std::any_of(actions.begin(), actions.end(),
			[&](const InputAction &action) { return action.name == name; });
	};
	std::string name = base;
	for (int suffix = 2; taken(name); ++suffix) {
		name = base + "_" + std::to_string(suffix);
	}
	return name;
}

} // namespace sowa::editor
