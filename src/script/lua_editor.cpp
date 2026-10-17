#include "script/lua_editor.hpp"

#include <sol/sol.hpp>
#include <stdexcept>

#include "core/command_inputs.hpp"
#include "core/commands.hpp"

namespace sowa {
namespace {

void checkResult(const sol::protected_function_result &result) {
	if (!result.valid()) {
		const sol::error error = result;
		throw std::runtime_error(error.what());
	}
}

CommandPrompt readPrompt(const std::string &kind, sol::table options) {
	CommandPrompt prompt;
	prompt.title = options.get<std::string>("title");
	prompt.placeholder = options.get_or("placeholder", std::string{});
	prompt.value = options.get_or("value", std::string{});

	if (kind == "select") {
		prompt.kind = CommandPrompt::Kind::Select;
		sol::table choices = options.get<sol::table>("options");
		for (std::size_t i = 1; i <= choices.size(); ++i) {
			sol::object item = choices[i];
			if (item.is<std::string>()) {
				const auto text = item.as<std::string>();
				prompt.options.push_back({text, text});
			} else {
				const auto choice = item.as<sol::table>();
				const auto label = choice.get<std::string>("label");
				prompt.options.push_back({label, choice.get_or("value", label)});
			}
		}
	} else if (kind != "text") {
		throw std::invalid_argument("Unknown command input type: " + kind);
	}

	const auto validate = options.get<sol::optional<sol::protected_function>>("validate");
	const bool required = options.get_or("required", false);
	prompt.validate = [validate, required](const std::string &value) -> std::string {
		if (required && value.find_first_not_of(" \t\r\n") == std::string::npos) {
			return "Enter a value to continue.";
		}
		if (validate) {
			const auto result = (*validate)(value);
			checkResult(result);
			return result.get<sol::optional<std::string>>().value_or("");
		}
		return {};
	};
	return prompt;
}

} // namespace

void BindEditorCommands(sol::state &lua, CommandRegistry &commands, CommandInputs &inputs,
	const std::vector<std::string> &sourcePaths) {
	auto api = lua.create_named_table("editor");
	auto commandApi = lua.create_table();
	api["commands"] = commandApi;

	commandApi.set_function("register", [&](sol::table definition) {
		PaletteCommand command;
		command.id = definition.get<std::string>("id");
		command.title = definition.get<std::string>("title");
		command.keywords = definition.get_or("keywords", std::string{});
		command.allowWhilePlaying = definition.get_or("allow_while_playing", false);
		if (command.id.starts_with("editor.")) {
			throw std::invalid_argument("The editor. command prefix is reserved");
		}
		sol::protected_function run = definition.get<sol::protected_function>("run");
		if (!run.valid()) {
			throw std::invalid_argument("Command run must be a function");
		}
		command.run = [run, &inputs, allowPlay = command.allowWhilePlaying] {
			inputs.Begin(allowPlay);
			checkResult(run());
		};
		if (!sourcePaths.empty()) {
			command.source = sourcePaths.back();
		}
		commands.Register(std::move(command));
	});
	commandApi.set_function("unregister",
		[&](const std::string &id) { return commands.Unregister(id); });

	// Lua handles coroutine suspension; C++ only owns the current prompt.
	api.set_function("_prompt",
		[&](const std::string &kind, sol::table options, sol::protected_function resume) {
			auto prompt = readPrompt(kind, options);
			prompt.submit = [resume](const std::string &value) { checkResult(resume(value)); };
			inputs.Request(std::move(prompt));
		});
	checkResult(lua.safe_script(R"lua(
		local register = editor.commands.register
		local prompt = editor._prompt
		editor._prompt = nil

		local function resume(thread, ...)
			local ok, kind, options = coroutine.resume(thread, ...)
			if not ok then error(kind, 0) end
			if coroutine.status(thread) == "dead" then return end
			prompt(kind, options, function(value) resume(thread, value) end)
		end

		function editor.commands.register(command)
			assert(type(command.run) == "function", "Command run must be a function")
			local run = command.run
			local definition = {}
			for key, value in pairs(command) do definition[key] = value end
			definition.run = function() resume(coroutine.create(run)) end
			register(definition)
		end

		editor.input = {}
		function editor.input.text(options)
			return coroutine.yield("text", options)
		end
		function editor.input.select(options)
			return coroutine.yield("select", options)
		end
	)lua",
		sol::script_pass_on_error, "editor input API"));
}

} // namespace sowa
