#include <doctest/doctest.h>

#include "core/filesystem/file_system.hpp"
#include "core/input.hpp"
#include "core/project_settings.hpp"
#include "ecs/world.hpp"
#include "editor/command_palette.hpp"
#include "imgui.h"
#include "imgui_internal.h"
#include "resource/resource_registry.hpp"
#include "scene/scene_manager.hpp"
#include "script/script_server.hpp"

using namespace sowa;

TEST_CASE("Command palette searches titles, identifiers and keywords") {
	CommandRegistry commands;
	commands.Register({"save", "Scene: Save", "write disk", "", [] {}});
	commands.Register({"reset", "Reset Camera", "viewport", "", [] {}});
	CHECK(commands.Search("").size() == 2);
	CHECK(commands.Search("SCENE save").at(0).id == "save");
	CHECK(commands.Search("disk").at(0).id == "save");
	CHECK(commands.Search("rscam").at(0).id == "reset");
	CHECK(commands.Search("reset").at(0).id == "reset");
	CHECK(commands.Search("missing").empty());
	CHECK(commands.Search("   ").size() == 2);
}

TEST_CASE("Command execution respects availability and permits registry mutation") {
	CommandRegistry commands;
	std::string error;
	int runs = 0;
	commands.Register({"test", "Test", "", "", [&] { ++runs; }});
	CHECK_FALSE(commands.Execute("test", true, error));
	CHECK(runs == 0);
	commands.Register({"test", "Replacement", "", "", [&] {
						   ++runs;
						   commands.Clear();
					   }});
	CHECK(commands.Search("").size() == 1);
	CHECK(commands.Execute("test", false, error));
	CHECK(error.empty());
	CHECK(runs == 1);
	CHECK(commands.Search("").empty());
	CHECK_FALSE(commands.Execute("missing", false, error));
	commands.Register({"disabled", "Disabled", "", "", [&] { ++runs; }, false, true});
	CHECK_FALSE(commands.Execute("disabled", false, error));
	CHECK(runs == 1);
	CHECK_THROWS(commands.Register({"", "Invalid", "", "", [] {}}));
	CHECK_THROWS(commands.Register({"invalid", "   ", "", "", [] {}}));
}

TEST_CASE("Lua commands validate registrations, replace, unregister and protect callbacks") {
	World world;
	ScriptServer scripts(world);
	REQUIRE(scripts.DoString(R"(
		count = 0
		editor.commands.register {
			id = "tools.hello", title = "Tools: Hello", keywords = "greet",
			run = function() count = count + 1 end
		}
	)"));
	std::string error;
	CHECK_FALSE(scripts.Commands().Execute("tools.hello", true, error));
	CHECK(scripts.Commands().Execute("tools.hello", false, error));
	CHECK(scripts.Lua().get<int>("count") == 1);
	REQUIRE(scripts.DoString(R"(
		editor.commands.register {
			id = "tools.hello", title = "Tools: Replacement", allow_while_playing = true,
			run = function()
				count = count + 10
				editor.commands.unregister("tools.hello")
			end
		}
	)"));
	CHECK(scripts.Commands().Search("").size() == 1);
	CHECK(scripts.Commands().Execute("tools.hello", true, error));
	CHECK(scripts.Lua().get<int>("count") == 11);
	CHECK(scripts.Commands().Search("").empty());
	REQUIRE(scripts.DoString(R"(
		editor.commands.register { id = "broken", title = "Broken", run = function() error("oops") end }
	)"));
	CHECK_FALSE(scripts.Commands().Execute("broken", false, error));
	CHECK(error.find("oops") != std::string::npos);
	CHECK_FALSE(scripts.DoString(
		R"(editor.commands.register { id = "editor.scene.save", title = "Override", run = function() end })"));
	CHECK_FALSE(scripts.DoString(
		R"(editor.commands.register { id = "invalid", title = "Invalid", run = 42 })"));
	CHECK_FALSE(
		scripts.DoString(R"(editor.commands.register { id = "invalid", run = function() end })"));
}

TEST_CASE("Palette focuses search, navigates filtered results, executes and dismisses") {
	ImGui::CreateContext();

	struct Cleanup {
		~Cleanup() {
			ImGui::DestroyContext();
		}
	} cleanup;

	auto &io = ImGui::GetIO();
	io.IniFilename = nullptr;
	io.DisplaySize = {1000.f, 700.f};
	io.DeltaTime = 1.f / 60.f;
	unsigned char *pixels;
	int width, height;
	io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
	CommandRegistry commands;
	commands.Register({"a", "Alpha", "", "", [] {}});
	commands.Register({"b", "Beta", "", "", [] {}});
	CommandInputs inputs;
	editor::CommandPalette palette;
	std::optional<std::string> chosen;
	const auto frame = [&] {
		ImGui::NewFrame();
		chosen = palette.Draw(commands, inputs, false);
		ImGui::Render();
	};
	const auto key = [&](ImGuiKey code) {
		io.AddKeyEvent(code, true);
		frame();
		const auto result = chosen;
		io.AddKeyEvent(code, false);
		frame();
		return result;
	};
	palette.Open();
	frame();
	frame();
	frame();
	CHECK(io.WantTextInput);
	key(ImGuiKey_DownArrow);
	CHECK(key(ImGuiKey_Enter) == std::optional<std::string>{"b"});
	palette.Open();
	frame();
	frame();
	io.AddInputCharactersUTF8("bet");
	frame();
	CHECK(key(ImGuiKey_Enter) == std::optional<std::string>{"b"});
	palette.Open();
	frame();
	frame();
	CHECK_FALSE(key(ImGuiKey_Escape).has_value());
	CHECK_FALSE(key(ImGuiKey_Enter).has_value());
}

TEST_CASE("Lua commands suspend for text and selection, then resume with values") {
	World world;
	ScriptServer scripts(world);
	auto &inputs = scripts.CommandInputs();
	REQUIRE(scripts.DoString(R"(
		editor.commands.register {
			id = "send", title = "Send message",
			run = function()
				local message = editor.input.text {
					title = "What message should be sent?",
					required = true,
					validate = function(value)
						if #value > 20 then return "Too long" end
					end,
				}
				local server = editor.input.select {
					title = "Which server to send to?",
					options = {
						"Server A",
						{ label = "Server B", value = "b" },
						"Server C",
					},
				}
				delivered = server .. ": " .. message
			end,
		}
	)"));
	std::string error;
	REQUIRE(scripts.Commands().Execute("send", false, error));
	REQUIRE(inputs.Current());
	CHECK(inputs.Current()->title == "What message should be sent?");
	CHECK_FALSE(inputs.Submit("  "));
	CHECK_FALSE(inputs.Error().empty());
	CHECK_FALSE(inputs.Submit(std::string(30, 'a')));
	CHECK(inputs.Error() == "Too long");
	REQUIRE(inputs.Submit("Hello"));
	REQUIRE(inputs.Current());
	CHECK(inputs.Current()->title == "Which server to send to?");
	CHECK(inputs.Current()->options.size() == 3);
	CHECK_FALSE(inputs.Submit("Unknown server"));
	REQUIRE(inputs.Submit("b"));
	CHECK_FALSE(inputs.Current());
	CHECK(scripts.Lua().get<std::string>("delivered") == "b: Hello");

	REQUIRE(scripts.Commands().Execute("send", false, error));
	inputs.Cancel();
	CHECK_FALSE(inputs.Submit("Must not send"));
	CHECK(scripts.Lua().get<std::string>("delivered") == "b: Hello");
}

TEST_CASE("Prompt errors release the workflow and a new command can run") {
	World world;
	ScriptServer scripts(world);
	REQUIRE(scripts.DoString(R"(
		editor.commands.register {
			id = "bad", title = "Bad command",
			run = function()
				editor.input.text { title = "Message" }
				error("failed after input")
			end,
		}
		editor.commands.register {
			id = "empty", title = "Empty options",
			run = function()
				editor.input.select { title = "Choose", options = {} }
			end,
		}
	)"));
	std::string error;
	REQUIRE(scripts.Commands().Execute("bad", false, error));
	CHECK_FALSE(scripts.CommandInputs().Submit("hello"));
	CHECK_FALSE(scripts.CommandInputs().Current());
	CHECK_FALSE(scripts.Commands().Execute("empty", false, error));
	CHECK(error.find("at least one option") != std::string::npos);
	CHECK_FALSE(scripts.CommandInputs().Current());
	REQUIRE(scripts.Commands().Execute("bad", false, error));
	CHECK(scripts.CommandInputs().Current());
}

TEST_CASE("Example project loads runnable command demonstrations") {
	World world;
	ScriptServer scripts(world);
	FileSystem fs;
	fs.MountFolder("res", SOWA_EXAMPLE_DIR);
	Input input;
	ResourceRegistry resources;
	SceneManager scenes(world, resources, fs);
	ProjectSettings settings;
	scripts.BindEngine(input, scenes, resources, settings);
	REQUIRE(scripts.LoadModules(fs, {"res://scripts/editor_commands.lua"}));
	CHECK(scripts.Commands().Search("example").size() == 3);
	std::string error;
	REQUIRE(scripts.Commands().Execute("example.send_message", false, error));
	REQUIRE(scripts.CommandInputs().Submit("Testing the example"));
	REQUIRE(scripts.CommandInputs().Current());
	CHECK(scripts.CommandInputs().Current()->options.size() == 3);
	REQUIRE(scripts.CommandInputs().Submit("Server B"));
	CHECK_FALSE(scripts.CommandInputs().Current());
}

TEST_CASE("Prompt UI stays compact, accepts long text and advances to server choices") {
	ImGui::CreateContext();

	struct Cleanup {
		~Cleanup() {
			ImGui::DestroyContext();
		}
	} cleanup;

	auto &io = ImGui::GetIO();
	io.IniFilename = nullptr;
	io.DisplaySize = {1000.f, 700.f};
	io.DeltaTime = 1.f / 60.f;
	unsigned char *pixels;
	int width, height;
	io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

	World world;
	ScriptServer scripts(world);
	REQUIRE(scripts.DoString(R"(
		editor.commands.register {
			id = "flow", title = "Flow",
			run = function()
				message = editor.input.text { title = "Message", required = true }
				server = editor.input.select {
					title = "Which server?", options = { "A", "B", "C" },
				}
			end,
		}
	)"));
	editor::CommandPalette palette;
	auto &inputs = scripts.CommandInputs();
	const auto frame = [&] {
		ImGui::NewFrame();
		palette.Draw(scripts.Commands(), inputs, false);
		ImGui::Render();
	};
	const auto key = [&](ImGuiKey code) {
		io.AddKeyEvent(code, true);
		frame();
		io.AddKeyEvent(code, false);
		frame();
	};
	std::string error;
	REQUIRE(scripts.Commands().Execute("flow", false, error));
	frame();
	frame();
	frame();
	REQUIRE(!ImGui::GetCurrentContext()->OpenPopupStack.empty());
	CHECK(ImGui::GetCurrentContext()->OpenPopupStack.back().Window->Size.y < 100.f);
	key(ImGuiKey_Enter);
	REQUIRE(inputs.Current());
	CHECK_FALSE(inputs.Error().empty());

	const std::string message(1000, 'a');
	io.AddInputCharactersUTF8(message.c_str());
	frame();
	const int typedLength = ImGui::GetCurrentContext()->InputTextState.TextLen;
	CAPTURE(typedLength);
	key(ImGuiKey_Enter);
	CAPTURE(inputs.Error());
	REQUIRE(inputs.Current());
	REQUIRE(inputs.Current()->kind == CommandPrompt::Kind::Select);
	frame();
	frame();
	CHECK(ImGui::GetCurrentContext()->OpenPopupStack.back().Window->Size.y < 150.f);
	key(ImGuiKey_DownArrow);
	key(ImGuiKey_Enter);
	CHECK_FALSE(inputs.Current());
	CHECK(scripts.Lua().get<std::string>("message") == message);
	CHECK(scripts.Lua().get<std::string>("server") == "B");

	REQUIRE(scripts.Commands().Execute("flow", false, error));
	frame();
	frame();
	key(ImGuiKey_Escape);
	CHECK_FALSE(inputs.Current());
	CHECK_FALSE(palette.IsOpen());
}
