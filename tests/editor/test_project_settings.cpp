#include <doctest/doctest.h>

#include "editor/project_settings.hpp"

using namespace sowa;

TEST_CASE("editable project settings equality includes the global store") {
	ProjectSettings a;
	ProjectSettings b;
	a.globalStore.Set("score", "10");
	b.globalStore.Set("score", "20");
	CHECK_FALSE(editor::EditableProjectSettingsEqual(a, b));
	b.globalStore.Set("score", "10");
	CHECK(editor::EditableProjectSettingsEqual(a, b));

	b.application.mainScene = "res://main.scene";
	CHECK_FALSE(editor::EditableProjectSettingsEqual(a, b));
	a.application.mainScene = b.application.mainScene;
	a.application.scripts = {"res://main.lua"};
	CHECK_FALSE(editor::EditableProjectSettingsEqual(a, b));
}

TEST_CASE("editable project settings validates dimensions and path lists") {
	ProjectSettings settings;
	CHECK(editor::ValidateEditableProjectSettings(settings).empty());

	settings.rendering.window.width = 0;
	CHECK(editor::ValidateEditableProjectSettings(settings).find("Window") !=
		  std::string::npos);
	settings.rendering.window.width = 1280;
	settings.rendering.viewport.height = -1;
	CHECK(editor::ValidateEditableProjectSettings(settings).find("Viewport") !=
		  std::string::npos);

	settings.rendering.viewport.height = 720;
	settings.application.scripts = {"res://main.lua", "res://main.lua"};
	CHECK(editor::ValidateEditableProjectSettings(settings).find("duplicate") !=
		  std::string::npos);
	settings.application.scripts.clear();
	settings.application.autoloads = {""};
	CHECK(editor::ValidateEditableProjectSettings(settings).find("empty") !=
		  std::string::npos);
}

TEST_CASE("editable project settings compare and validate input actions") {
	ProjectSettings a;
	ProjectSettings b;
	b.input.actions = {{"jump", {{Key::Space}}}};
	CHECK_FALSE(editor::EditableProjectSettingsEqual(a, b));
	a.input.actions = b.input.actions;
	CHECK(editor::EditableProjectSettingsEqual(a, b));
	CHECK(editor::ValidateEditableProjectSettings(b).empty());

	b.input.actions.push_back({"jump", {}});
	CHECK(editor::ValidateEditableProjectSettings(b).find("duplicate action") !=
		  std::string::npos);
	b.input.actions.back().name = "";
	CHECK(editor::ValidateEditableProjectSettings(b).find("empty") != std::string::npos);
	b.input.actions.back().name = "fire";
	CHECK(editor::ValidateEditableProjectSettings(b).empty());
}

TEST_CASE("shared bindings and unique action names") {
	const std::vector<InputAction> actions = {
		{"jump", {{Key::Space}, {Key::W}}},
		{"confirm", {{Key::Space}}},
		{"new_action", {}},
		{"new_action_2", {{Key::Space, InputModifierShift}}},
	};
	CHECK(editor::ActionsSharingBinding(actions, 0, {Key::Space}) ==
		  std::vector<std::string>{"confirm"});
	CHECK(editor::ActionsSharingBinding(actions, 0, {Key::W}).empty());
	CHECK(editor::ActionsSharingBinding(actions, 2, {Key::Space}) ==
		  std::vector<std::string>{"jump", "confirm"});

	CHECK(editor::UniqueActionName(actions, "fire") == "fire");
	CHECK(editor::UniqueActionName(actions, "new_action") == "new_action_3");
}
