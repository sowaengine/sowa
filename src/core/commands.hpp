#pragma once

#include <functional>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace sowa {

bool MatchesCommand(std::string_view text, std::string_view query);

struct PaletteCommand {
	std::string id;
	std::string title;
	std::string keywords;
	std::string shortcut;
	std::function<void()> run;
	bool enabled = true;
	bool allowWhilePlaying = false;
	std::string source;
};

// Owns callbacks; copy a command before running it so callbacks may safely
// unregister themselves or replace the registry during a script reload.
class CommandRegistry {
  public:
	PaletteCommand &Register(PaletteCommand command);
	bool Unregister(std::string_view id);
	void RemoveSource(std::string_view source);
	void Clear();
	std::vector<PaletteCommand> Search(std::string_view query) const;
	bool Execute(std::string_view id, bool playing, std::string &error) const;

  private:
	std::map<std::string, PaletteCommand, std::less<>> _commands;
};

} // namespace sowa
