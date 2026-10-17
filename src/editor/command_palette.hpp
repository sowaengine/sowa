#pragma once

#include <optional>
#include <string>
#include <vector>

#include "core/command_inputs.hpp"
#include "core/commands.hpp"

namespace sowa::editor {

class CommandPalette {
  public:
	void Open() {
		_openRequested = true;
	}

	bool IsOpen() const {
		return _openRequested || _visible;
	}

	std::optional<std::string> Draw(const CommandRegistry &commands, CommandInputs &inputs,
		bool playing);

  private:
	void resetSearch(const std::string &value = "");
	bool drawSearch(const char *placeholder);

	bool _openRequested = false;
	bool _visible = false;
	bool _closeRequested = false;
	unsigned _promptRevision = 0;
	std::vector<char> _text = std::vector<char>(256);
	int _selected = 0;
	int _navigation = 0;
};

} // namespace sowa::editor
