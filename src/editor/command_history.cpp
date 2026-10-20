#include "editor/command_history.hpp"

#include <algorithm>
#include <stdexcept>
#include <utility>

namespace sowa::editor {

CommandHistory::CommandHistory(bool initiallySaved, std::size_t maxEntries)
	: _maxEntries(maxEntries) {
	if (initiallySaved) {
		_savedState = _currentState;
	}
}

CommandHistory::~CommandHistory() {
	Clear();
}

CommandHistory::CommandHistory(CommandHistory &&other) noexcept
	: _entries(std::move(other._entries)), _cursor(other._cursor),
	  _currentState(other._currentState), _nextState(other._nextState),
	  _savedState(other._savedState), _maxEntries(other._maxEntries) {
	other._cursor = 0;
	other._savedState.reset();
}

CommandHistory &CommandHistory::operator=(CommandHistory &&other) noexcept {
	if (this == &other) {
		return *this;
	}
	Clear();
	_entries = std::move(other._entries);
	_cursor = other._cursor;
	_currentState = other._currentState;
	_nextState = other._nextState;
	_savedState = other._savedState;
	_maxEntries = other._maxEntries;
	other._cursor = 0;
	other._savedState.reset();
	return *this;
}

bool CommandHistory::Execute(std::unique_ptr<EditCommand> command) {
	if (!command || !command->Redo()) {
		return false;
	}
	return PushApplied(std::move(command));
}

bool CommandHistory::PushApplied(std::unique_ptr<EditCommand> command) {
	if (!command) {
		return false;
	}
	discardRedo();
	Entry entry;
	entry.command = std::move(command);
	entry.beforeState = _currentState;
	entry.afterState = _nextState++;
	_currentState = entry.afterState;
	_entries.push_back(std::move(entry));
	_cursor = _entries.size();
	trimOldest();
	return true;
}

bool CommandHistory::Undo() {
	if (!CanUndo()) {
		return false;
	}
	Entry &entry = _entries[_cursor - 1];
	if (!entry.command->Undo()) {
		return false;
	}
	--_cursor;
	_currentState = entry.beforeState;
	return true;
}

bool CommandHistory::Redo() {
	if (!CanRedo()) {
		return false;
	}
	Entry &entry = _entries[_cursor];
	if (!entry.command->Redo()) {
		return false;
	}
	++_cursor;
	_currentState = entry.afterState;
	return true;
}

void CommandHistory::discardRedo() {
	for (std::size_t i = _cursor; i < _entries.size(); ++i) {
		_entries[i].command->Discard();
	}
	_entries.erase(_entries.begin() + static_cast<std::ptrdiff_t>(_cursor), _entries.end());
}

void CommandHistory::trimOldest() {
	if (_entries.size() <= _maxEntries) {
		return;
	}
	const std::size_t removeCount = _entries.size() - _maxEntries;
	for (std::size_t i = 0; i < removeCount; ++i) {
		_entries[i].command->Discard();
	}
	_entries.erase(_entries.begin(), _entries.begin() + static_cast<std::ptrdiff_t>(removeCount));
	_cursor -= std::min(_cursor, removeCount);
}

void CommandHistory::Clear() {
	for (Entry &entry : _entries) {
		entry.command->Discard();
	}
	_entries.clear();
	_cursor = 0;
	_currentState = _nextState++;
	_savedState.reset();
}

std::string_view CommandHistory::UndoName() const {
	return CanUndo() ? _entries[_cursor - 1].command->Name() : std::string_view();
}

std::string_view CommandHistory::RedoName() const {
	return CanRedo() ? _entries[_cursor].command->Name() : std::string_view();
}

bool CommandBatch::Redo() {
	for (std::size_t i = 0; i < _commands.size(); ++i) {
		if (_commands[i]->Redo()) {
			continue;
		}
		while (i > 0) {
			--i;
			if (!_commands[i]->Undo()) {
				throw std::runtime_error("Editor batch rollback failed");
			}
		}
		return false;
	}
	return true;
}

bool CommandBatch::Undo() {
	for (std::size_t i = _commands.size(); i > 0; --i) {
		if (_commands[i - 1]->Undo()) {
			continue;
		}
		for (std::size_t j = i; j < _commands.size(); ++j) {
			if (!_commands[j]->Redo()) {
				throw std::runtime_error("Editor batch rollback failed");
			}
		}
		return false;
	}
	return true;
}

void CommandBatch::Discard() {
	for (auto &command : _commands) {
		command->Discard();
	}
	_commands.clear();
}
} // namespace sowa::editor
