#pragma once

#include <optional>

#include "core/input.hpp"

namespace sowa::editor {

// Captures raw input for the binding recorder. Key/mouse presses complete capture;
// a modifier alone completes on release. Ignores releases of keys held before Begin.
// Escape is recordable; closing the dialog cancels listening.
class BindingCapture {
  public:
	enum class State {
		Idle,
		Listening,
		Captured,
		Cancelled,
	};

	void Begin();
	void Cancel();
	void Feed(const InputEvent &event);

	State GetState() const {
		return _state;
	}

	bool IsListening() const {
		return _state == State::Listening;
	}

	// Returns the binding once Captured and goes back to Idle. Cancelled (see
	// Cancel) also goes back to Idle and returns nullopt; Listening/Idle
	// return nullopt without changing state.
	std::optional<InputBinding> Take();

  private:
	void finish(InputBinding binding);

	State _state = State::Idle;
	InputBinding _result;
	Key _pendingModifier = Key::Unknown;
};

} // namespace sowa::editor
