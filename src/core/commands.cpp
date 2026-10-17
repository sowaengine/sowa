#include "core/commands.hpp"

#include <algorithm>
#include <cctype>
#include <sstream>
#include <stdexcept>
#include <utility>

namespace sowa {
namespace {
std::string lower(std::string_view text) {
	std::string result(text);
	for (char &c : result) {
		c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
	}
	return result;
}

} // namespace

bool MatchesCommand(std::string_view text, std::string_view query) {
	const auto haystack = lower(text);
	std::istringstream words(lower(query));
	std::string word;
	while (words >> word) {
		std::size_t position = 0;
		for (char character : word) {
			position = haystack.find(character, position);
			if (position == std::string::npos) {
				return false;
			}
			++position;
		}
	}
	return true;
}

PaletteCommand &CommandRegistry::Register(PaletteCommand command) {
	if (command.id.empty() || command.title.find_first_not_of(" \t\r\n") == std::string::npos ||
		!command.run) {
		throw std::invalid_argument("Commands need an id, a title, and a run callback");
	}
	const auto id = command.id;
	return _commands.insert_or_assign(id, std::move(command)).first->second;
}

bool CommandRegistry::Unregister(std::string_view id) {
	return _commands.erase(std::string(id)) != 0;
}

void CommandRegistry::RemoveSource(std::string_view source) {
	std::erase_if(_commands, [source](const auto &entry) { return entry.second.source == source; });
}

void CommandRegistry::Clear() {
	_commands.clear();
}

std::vector<PaletteCommand> CommandRegistry::Search(std::string_view query) const {
	const auto needle = lower(query);
	std::vector<PaletteCommand> result;
	for (const auto &[id, command] : _commands) {
		if (MatchesCommand(command.title + " " + id + " " + command.keywords, needle)) {
			result.push_back(command);
		}
	}
	std::sort(result.begin(), result.end(), [&](const auto &a, const auto &b) {
		const auto left = lower(a.title), right = lower(b.title);
		const auto rank = [&](const std::string &title) {
			if (needle.empty() || title.starts_with(needle)) {
				return 0;
			}
			return title.find(needle) != std::string::npos ? 1 : 2;
		};
		if (rank(left) != rank(right)) {
			return rank(left) < rank(right);
		}
		return left != right ? left < right : a.id < b.id;
	});
	return result;
}

bool CommandRegistry::Execute(std::string_view id, bool playing, std::string &error) const {
	error.clear();
	const auto found = _commands.find(id);
	if (found == _commands.end()) {
		error = "Command is no longer registered";
		return false;
	}
	const auto command = found->second;
	if (!command.enabled || (playing && !command.allowWhilePlaying)) {
		error = "Command is unavailable in the current editor state";
		return false;
	}
	try {
		command.run();
		return true;
	} catch (const std::exception &exception) {
		error = exception.what();
	} catch (...) {
		error = "Unknown command error";
	}
	return false;
}

} // namespace sowa
