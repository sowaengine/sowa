#pragma once

#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "core/types.hpp"

namespace sowa::editor {

// One reversible editor mutation. Commands own any state needed to undo
// themselves (including parked entities) and release it from Discard when it
// is no longer reachable from the history.
class EditCommand {
  public:
	virtual ~EditCommand() = default;
	virtual std::string_view Name() const = 0;
	virtual bool Undo() = 0;
	virtual bool Redo() = 0;

	virtual void Discard() {
	}
};

// Undo/redo states have unique identities so a new branch cannot match a previously saved
// state.
class CommandHistory {
  public:
	static constexpr std::size_t kDefaultMaxEntries = 512;
	explicit CommandHistory(bool initiallySaved = true,
		std::size_t maxEntries = kDefaultMaxEntries);
	~CommandHistory();

	CommandHistory(const CommandHistory &) = delete;
	CommandHistory &operator=(const CommandHistory &) = delete;
	CommandHistory(CommandHistory &&other) noexcept;
	CommandHistory &operator=(CommandHistory &&other) noexcept;

	// Execute a new command, then append it if execution succeeded.
	bool Execute(std::unique_ptr<EditCommand> command);
	// Append a mutation the UI already applied (for ImGui live editing).
	bool PushApplied(std::unique_ptr<EditCommand> command);
	bool Undo();
	bool Redo();
	void Clear();

	bool CanUndo() const {
		return _cursor > 0;
	}

	bool CanRedo() const {
		return _cursor < _entries.size();
	}

	bool IsDirty() const {
		return !_savedState.has_value() || *_savedState != _currentState;
	}

	void MarkSaved() {
		_savedState = _currentState;
	}

	std::string_view UndoName() const;
	std::string_view RedoName() const;

  private:
	struct Entry {
		std::unique_ptr<EditCommand> command;
		u64 beforeState = 0;
		u64 afterState = 0;
	};

	void discardRedo();
	void trimOldest();

	std::vector<Entry> _entries;
	std::size_t _cursor = 0;
	u64 _currentState = 1;
	u64 _nextState = 2;
	std::optional<u64> _savedState;
	std::size_t _maxEntries = kDefaultMaxEntries;
};

// Children must leave their own state unchanged on failure. Successfully applied
// siblings are rolled back; failed history movement never advances the cursor.
class CommandBatch final : public EditCommand {
  public:
	explicit CommandBatch(std::string name) : _name(std::move(name)) {
	}

	~CommandBatch() override {
		Discard();
	}

	void Add(std::unique_ptr<EditCommand> command) {
		_commands.push_back(std::move(command));
	}

	bool Empty() const {
		return _commands.empty();
	}

	std::string_view Name() const override {
		return _name;
	}

	bool Undo() override;
	bool Redo() override;
	void Discard() override;

  private:
	std::string _name;
	std::vector<std::unique_ptr<EditCommand>> _commands;
};
} // namespace sowa::editor
