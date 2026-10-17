#include "core/command_inputs.hpp"

#include <algorithm>
#include <stdexcept>
#include <utility>

#include "core/log.hpp"

namespace sowa {

void CommandInputs::Request(CommandPrompt prompt) {
	if (prompt.title.empty() || !prompt.submit) {
		throw std::invalid_argument("A command prompt needs a title and a continuation");
	}
	if (prompt.kind == CommandPrompt::Kind::Select && prompt.options.empty()) {
		throw std::invalid_argument("A selection prompt needs at least one option");
	}
	_prompt = std::move(prompt);
	_error.clear();
	++_revision;
}

bool CommandInputs::Submit(const std::string &value) {
	if (!_prompt) {
		return false;
	}
	// Copy before invoking script code: it may cancel or replace the prompt.
	const auto prompt = *_prompt;
	try {
		if (prompt.kind == CommandPrompt::Kind::Select) {
			const auto found = std::find_if(prompt.options.begin(), prompt.options.end(),
				[&](const CommandChoice &choice) { return choice.value == value; });
			if (found == prompt.options.end()) {
				_error = "Choose an option from the list.";
				return false;
			}
		}
		_error = prompt.validate ? prompt.validate(value) : "";
		if (!_error.empty()) {
			return false;
		}
		Cancel();
		prompt.submit(value);
		return true;
	} catch (const std::exception &error) {
		Cancel();
		Log::Error("Command input: {}", error.what());
		return false;
	}
}

void CommandInputs::Cancel() {
	_prompt.reset();
	_error.clear();
}

} // namespace sowa
