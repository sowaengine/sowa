#pragma once

#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace sowa {

struct CommandChoice {
	std::string label;
	std::string value;
};

struct CommandPrompt {
	enum class Kind {
		Text,
		Select
	};

	Kind kind = Kind::Text;
	std::string title;
	std::string placeholder;
	std::string value;
	std::vector<CommandChoice> options;
	std::function<std::string(const std::string &)> validate;
	std::function<void(const std::string &)> submit;
};

// A single pending step. Submitting may request the next step; cancellation
// drops the continuation so the rest of the command does not run.
class CommandInputs {
  public:
	void Begin(bool allowWhilePlaying) {
		Cancel();
		_allowWhilePlaying = allowWhilePlaying;
	}

	bool AllowsPlay() const {
		return _allowWhilePlaying;
	}

	void Request(CommandPrompt prompt);
	bool Submit(const std::string &value);
	void Cancel();

	const CommandPrompt *Current() const {
		return _prompt ? &*_prompt : nullptr;
	}

	const std::string &Error() const {
		return _error;
	}

	unsigned Revision() const {
		return _revision;
	}

  private:
	std::optional<CommandPrompt> _prompt;
	std::string _error;
	unsigned _revision = 0;
	bool _allowWhilePlaying = false;
};

} // namespace sowa
