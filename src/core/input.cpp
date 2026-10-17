#include "core/input.hpp"

#include <algorithm>
#include <utility>

#include "render/viewport_scale.hpp"

namespace sowa {
namespace {

const std::vector<InputBinding> kNoBindings;

} // namespace

void Input::SetViewportMapping(const Rect &presentRect, Vector2 contentSize,
	Vector2 framebufferSize, Vector2 viewportSize) {
	_presentRect = presentRect;
	_contentSize = contentSize;
	_framebufferSize = framebufferSize;
	_viewportSize = viewportSize;
	_hasViewportMapping = true;
}

Vector2 Input::GetMousePosition() const {
	return MapWindowPosition(_cursorPos);
}

Vector2 Input::MapWindowPosition(Vector2 position) const {
	if (!_hasViewportMapping) {
		return position;
	}
	return MapMouseToViewport(position, _contentSize, _framebufferSize, _presentRect,
		_viewportSize);
}

bool Input::IsMouseOverViewport() const {
	if (!_hasViewportMapping) {
		return false;
	}
	const f32 scaleX = _framebufferSize.x / std::max(_contentSize.x, 1.f);
	const f32 scaleY = _framebufferSize.y / std::max(_contentSize.y, 1.f);
	Vector2 fbPos(_cursorPos.x * scaleX, _cursorPos.y * scaleY);
	fbPos.y = _framebufferSize.y - fbPos.y;
	return _presentRect.Contains(fbPos);
}

void Input::Update() {
	_uiPointer = _uiKeyboard = false;
	for (auto &[key, state] : _keyStates) {
		if (state == KeyState::JustPressed) {
			state = KeyState::Down;
		} else if (state == KeyState::JustReleased) {
			state = KeyState::Up;
		}
	}
	std::erase_if(_editorKeys,
		[&](i32 key) { return stateOf(static_cast<Key>(key)) == KeyState::Up; });
	_scrollDelta = Vector2(0.f, 0.f);
}

void Input::SetEditorCapture(bool pointer, bool keyboard) {
	_editorPointer = pointer;
	_editorKeyboard = keyboard;
	// Capture new presses until physical release; preserve keys already held by the game.
	for (const auto &[key, state] : _keyStates) {
		if ((key >= 10001 ? pointer : keyboard) && state == KeyState::JustPressed) {
			_editorKeys.insert(key);
		}
	}
}

void Input::BeginEventBatch() {
	_batchEvents = true;
}

void Input::DispatchEvents() {
	_batchEvents = false;
	std::vector<InputEvent> events;
	events.swap(_pendingEvents);
	for (const auto &event : events) {
		dispatch(event);
	}
}

void Input::emit(const InputEvent &event) {
	_unfilteredEvents(event);
	if (_batchEvents) {
		_pendingEvents.push_back(event);
	} else {
		dispatch(event);
	}
}

void Input::dispatch(const InputEvent &event) {
	switch (event.type) {
	case InputEventType::Key:
		if (_editorKeyboard || _editorKeys.contains(static_cast<i32>(event.key.key))) {
			return;
		}
		break;
	case InputEventType::Char:
		if (_editorKeyboard) {
			return;
		}
		break;
	case InputEventType::MouseButton:
		if (_editorPointer || _editorKeys.contains(static_cast<i32>(event.button.button))) {
			return;
		}
		break;
	case InputEventType::MouseMove:
	case InputEventType::Scroll:
		if (_editorPointer) {
			return;
		}
		break;
	}
	_events(event);
	if (event.type == InputEventType::Char) {
		_charEvents(event.character.codepoint);
	}
}

Input::KeyState Input::stateOf(Key key) const {
	const auto it = _keyStates.find(static_cast<i32>(key));
	if (it == _keyStates.end()) {
		return KeyState::Up;
	}
	return it->second;
}

void Input::setKeyState(Key key, KeyAction action) {
	if (action == KeyAction::Repeat) {
		return;
	}

	const i32 id = static_cast<i32>(key);
	if (!_batchEvents && (id >= 10001 ? _editorPointer : _editorKeyboard) &&
		action == KeyAction::Press) {
		_editorKeys.insert(id);
	}
	if (action == KeyAction::Press) {
		_keyStates[id] = KeyState::JustPressed;
	} else if (action == KeyAction::Release) {
		_keyStates[id] = KeyState::JustReleased;
	}
}

bool Input::IsKeyDown(Key key) const {
	if (IsCapturedByUI(key)) {
		return false;
	}
	const KeyState state = stateOf(key);
	return state == KeyState::Down || state == KeyState::JustPressed;
}

bool Input::IsKeyJustPressed(Key key) const {
	if (IsCapturedByUI(key)) {
		return false;
	}
	return stateOf(key) == KeyState::JustPressed;
}

bool Input::IsKeyJustReleased(Key key) const {
	if (IsCapturedByUI(key)) {
		return false;
	}
	return stateOf(key) == KeyState::JustReleased;
}

void Input::SetActions(const std::vector<InputAction> &actions) {
	_actions.clear();
	for (const InputAction &action : actions) {
		_actions[action.name] = action.bindings;
	}
}

void Input::SetActionBindings(std::string_view name, std::vector<InputBinding> bindings) {
	_actions[std::string(name)] = std::move(bindings);
}

const std::vector<InputBinding> &Input::GetActionBindings(std::string_view name) const {
	const auto it = _actions.find(std::string(name));
	if (it == _actions.end()) {
		return kNoBindings;
	}
	return it->second;
}

bool Input::isHeld(Key key, bool previous) const {
	const KeyState state = stateOf(key);
	if (previous) {
		return state == KeyState::Down || state == KeyState::JustReleased;
	}
	return state == KeyState::Down || state == KeyState::JustPressed;
}

bool Input::isBindingActive(const InputBinding &binding, bool previous) const {
	// Capture is checked on the bound key only, through the same check the
	// key queries use. Modifiers are read raw: a captured modifier still
	// counts as held, and the bound key's own capture is what blocks input.
	if (IsCapturedByUI(binding.key) || !isHeld(binding.key, previous)) {
		return false;
	}
	constexpr std::pair<u8, std::pair<Key, Key>> kModifierKeys[] = {
		{InputModifierShift, {Key::LeftShift, Key::RightShift}},
		{InputModifierControl, {Key::LeftControl, Key::RightControl}},
		{InputModifierAlt, {Key::LeftAlt, Key::RightAlt}},
		{InputModifierSuper, {Key::LeftSuper, Key::RightSuper}},
	};
	for (const auto &[bit, keys] : kModifierKeys) {
		if ((binding.modifiers & bit) != 0 && !isHeld(keys.first, previous) &&
			!isHeld(keys.second, previous)) {
			return false;
		}
	}
	return true;
}

bool Input::isActionActive(std::string_view name, bool previous) const {
	for (const InputBinding &binding : GetActionBindings(name)) {
		if (isBindingActive(binding, previous)) {
			return true;
		}
	}
	return false;
}

bool Input::IsActionPressed(std::string_view name) const {
	return isActionActive(name, false);
}

bool Input::IsActionJustPressed(std::string_view name) const {
	return isActionActive(name, false) && !isActionActive(name, true);
}

bool Input::IsActionJustReleased(std::string_view name) const {
	return !isActionActive(name, false) && isActionActive(name, true);
}

f32 Input::GetActionStrength(std::string_view name) const {
	return IsActionPressed(name) ? 1.f : 0.f;
}

f32 Input::GetAxis(std::string_view negative, std::string_view positive) const {
	return GetActionStrength(positive) - GetActionStrength(negative);
}

Vector2 Input::GetVector(std::string_view negativeX, std::string_view positiveX,
	std::string_view negativeY, std::string_view positiveY) const {
	return Vector2(GetAxis(negativeX, positiveX), GetAxis(negativeY, positiveY));
}

void Input::HandleKey(Key key, KeyAction action, i32 mods) {
	setKeyState(key, action);

	InputEvent event;
	event.type = InputEventType::Key;
	event.key.key = key;
	event.key.action = action;
	event.key.mods = mods;
	event.windowPosition = _cursorPos;
	emit(event);
}

void Input::HandleMouseButton(i32 button, KeyAction action, i32 mods) {
	const Key key = static_cast<Key>(static_cast<i32>(Key::Mouse1) + button);
	setKeyState(key, action);

	InputEvent event;
	event.type = InputEventType::MouseButton;
	event.button.button = key;
	event.button.action = action;
	event.button.mods = mods;
	event.windowPosition = _cursorPos;
	emit(event);
}

void Input::HandleCursorPos(f32 x, f32 y) {
	InputEvent event;
	event.type = InputEventType::MouseMove;
	event.mouseMove.deltaX = _cursorPos.x - x;
	event.mouseMove.deltaY = _cursorPos.y - y;
	event.mouseMove.x = x;
	event.mouseMove.y = y;

	_cursorPos.x = x;
	_cursorPos.y = y;

	event.windowPosition = _cursorPos;
	emit(event);
}

void Input::HandleScroll(f32 xOffset, f32 yOffset) {
	_scrollDelta.x = xOffset;
	_scrollDelta.y = yOffset;

	InputEvent event;
	event.type = InputEventType::Scroll;
	event.scroll.xOffset = xOffset;
	event.scroll.yOffset = yOffset;
	event.windowPosition = _cursorPos;
	emit(event);
}

void Input::HandleChar(u32 codepoint) {
	InputEvent event;
	event.type = InputEventType::Char;
	event.character.codepoint = codepoint;
	event.windowPosition = _cursorPos;
	emit(event);
}

} // namespace sowa
