#pragma once

#include <sol/forward.hpp>
#include <string>
#include <vector>

namespace sowa {
class CommandInputs;
class CommandRegistry;

void BindEditorCommands(sol::state &lua, CommandRegistry &commands, CommandInputs &inputs,
	const std::vector<std::string> &sourcePaths);

} // namespace sowa
