#include "editor/binding_capture.hpp"

#include "core/input_binding.hpp"

namespace sowa::editor {

void BindingCapture::Begin() {
	_state = State::Listening;
	_result = {};
	_pendingModifier = Key::Unknown;
}

void BindingCapture::Cancel() {
	if (_state != State::Idle) {
		_state = State::Cancelled;
	}
}

void BindingCapture::finish(InputBinding binding) {
	_result = binding;
	_state = State::Captured;
}

void BindingCapture::Feed(const InputEvent &event) {
	if (_state != State::Listening) {
		return;
	}

	if (event.type == InputEventType::MouseButton) {
		if (event.button.action == KeyAction::Press) {
			finish({event.button.button, static_cast<u8>(event.button.mods & InputModifierMask)});
		}
		return;
	}
	if (event.type != InputEventType::Key) {
		return;
	}

	const Key key = event.key.key;
	const u8 modifiers = static_cast<u8>(event.key.mods & InputModifierMask);
	if (event.key.action == KeyAction::Press) {
		if (ModifierOfKey(key) != 0) {
			_pendingModifier = key;
		} else if (KeyName(key).empty()) {
			// Keys without a name cannot be written to project.sowa.
			return;
		} else {
			finish({key, modifiers});
		}
	} else if (event.key.action == KeyAction::Release && key == _pendingModifier) {
		finish({key, 0});
	}
}

std::optional<InputBinding> BindingCapture::Take() {
	if (_state == State::Captured) {
		_state = State::Idle;
		return _result;
	}
	if (_state == State::Cancelled) {
		_state = State::Idle;
	}
	return std::nullopt;
}

} // namespace sowa::editor
