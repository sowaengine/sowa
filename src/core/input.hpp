#pragma once

#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <eventpp/callbacklist.h>

#include "core/types.hpp"
#include "math/rect.hpp"
#include "math/vector2.hpp"

namespace sowa {

// Key codes match GLFW's values so they map 1:1 with the window backend.
// Mouse buttons live in a high range
enum class Key : i32 {
	Unknown = -1,

	Space = 32,
	Apostrophe = 39,
	Comma = 44,
	Minus = 45,
	Period = 46,
	Slash = 47,
	Zero = 48,
	One = 49,
	Two = 50,
	Three = 51,
	Four = 52,
	Five = 53,
	Six = 54,
	Seven = 55,
	Eight = 56,
	Nine = 57,
	Semicolon = 59,
	Equal = 61,
	A = 65,
	B = 66,
	C = 67,
	D = 68,
	E = 69,
	F = 70,
	G = 71,
	H = 72,
	I = 73,
	J = 74,
	K = 75,
	L = 76,
	M = 77,
	N = 78,
	O = 79,
	P = 80,
	Q = 81,
	R = 82,
	S = 83,
	T = 84,
	U = 85,
	V = 86,
	W = 87,
	X = 88,
	Y = 89,
	Z = 90,
	LeftBracket = 91,
	Backslash = 92,
	RightBracket = 93,
	GraveAccent = 96,
	World1 = 161,
	World2 = 162,

	Escape = 256,
	Enter = 257,
	Tab = 258,
	Backspace = 259,
	Insert = 260,
	Delete = 261,
	Right = 262,
	Left = 263,
	Down = 264,
	Up = 265,
	PageUp = 266,
	PageDown = 267,
	Home = 268,
	End = 269,
	CapsLock = 280,
	ScrollLock = 281,
	NumLock = 282,
	PrintScreen = 283,
	Pause = 284,
	F1 = 290,
	F2 = 291,
	F3 = 292,
	F4 = 293,
	F5 = 294,
	F6 = 295,
	F7 = 296,
	F8 = 297,
	F9 = 298,
	F10 = 299,
	F11 = 300,
	F12 = 301,
	F13 = 302,
	F14 = 303,
	F15 = 304,
	F16 = 305,
	F17 = 306,
	F18 = 307,
	F19 = 308,
	F20 = 309,
	F21 = 310,
	F22 = 311,
	F23 = 312,
	F24 = 313,
	F25 = 314,
	Keypad0 = 320,
	Keypad1 = 321,
	Keypad2 = 322,
	Keypad3 = 323,
	Keypad4 = 324,
	Keypad5 = 325,
	Keypad6 = 326,
	Keypad7 = 327,
	Keypad8 = 328,
	Keypad9 = 329,
	KeypadDecimal = 330,
	KeypadDivide = 331,
	KeypadMultiply = 332,
	KeypadSubtract = 333,
	KeypadAdd = 334,
	KeypadEnter = 335,
	KeypadEqual = 336,
	LeftShift = 340,
	LeftControl = 341,
	LeftAlt = 342,
	LeftSuper = 343,
	RightShift = 344,
	RightControl = 345,
	RightAlt = 346,
	RightSuper = 347,
	Menu = 348,

	Mouse1 = 10001,
	Mouse2 = 10002,
	Mouse3 = 10003,
	Mouse4 = 10004,
	Mouse5 = 10005,
	Mouse6 = 10006,
	Mouse7 = 10007,
	Mouse8 = 10008,
	MouseLeft = Mouse1,
	MouseRight = Mouse2,
	MouseMiddle = Mouse3,
};

// Modifier bits for InputBinding::modifiers. Values match GLFW's mods bits,
// so an InputEvent's mods can be masked straight into a binding.
enum InputModifier : u8 {
	InputModifierShift = 1 << 0,
	InputModifierControl = 1 << 1,
	InputModifierAlt = 1 << 2,
	InputModifierSuper = 1 << 3,
	InputModifierMask = 0x0F,
};

// Key or mouse button with required modifiers. Extra held modifiers are allowed,
// so an unmodified S binding also fires while Ctrl is held.
// Serialized as text such as "Space", "Ctrl+S", or "Shift+MouseLeft"; see input_binding.hpp.
struct InputBinding {
	Key key = Key::Unknown;
	u8 modifiers = 0;

	bool operator==(const InputBinding &) const = default;
};

struct InputAction {
	std::string name;
	// Any binding activates the action.
	std::vector<InputBinding> bindings;

	bool operator==(const InputAction &) const = default;
};

enum class KeyAction : i32 {
	Release = 0,
	Press = 1,
	Repeat = 2,
};

enum class InputEventType {
	Key,
	MouseButton,
	MouseMove,
	Scroll,
	Char,
};

struct InputEvent {
	InputEventType type{};
	Vector2 windowPosition;

	struct {
		Key key = Key::Unknown;
		KeyAction action = KeyAction::Release;
		i32 mods = 0;
	} key;

	struct {
		Key button = Key::Unknown;
		KeyAction action = KeyAction::Release;
		i32 mods = 0;
	} button;

	struct {
		f32 x = 0.f;
		f32 y = 0.f;
		f32 deltaX = 0.f;
		f32 deltaY = 0.f;
	} mouseMove;

	struct {
		f32 xOffset = 0.f;
		f32 yOffset = 0.f;
	} scroll;

	struct {
		u32 codepoint = 0;
	} character;
};

// Keyboard / mouse polling + event fan-out + named action mapping.
// Window forwards GLFW callbacks into Handle*; call Update() once per frame
// (separately from Window::PollEvents) to advance edge states and clear scroll.
class Input {
  public:
	using EventCallbackList = eventpp::CallbackList<void(const InputEvent &)>;
	using CharCallbackList = eventpp::CallbackList<void(u32 codepoint)>;

	// Retires key edges and clears scroll after consumers have read them.
	// Call before receiving new events, or at the end of the frame as Application does.
	void Update();

	// Batch window events until the editor has processed this frame's input.
	void BeginEventBatch();
	void DispatchEvents();
	void SetEditorCapture(bool pointer, bool keyboard);

	bool IsKeyDown(Key key) const;
	bool IsKeyJustPressed(Key key) const;
	bool IsKeyJustReleased(Key key) const;

	// Raw GLFW cursor position (content coords, origin top-left, Y-down).
	Vector2 GetWindowMousePosition() const {
		return _cursorPos;
	}

	// Mouse in game-viewport coordinates (origin bottom-left, Y-up).
	// Requires SetViewportMapping to have been called this frame.
	Vector2 GetMousePosition() const;
	Vector2 MapWindowPosition(Vector2 position) const;

	void SetUICapture(bool pointer, bool keyboard) {
		_uiPointer = pointer;
		_uiKeyboard = keyboard;
	}

	bool IsCapturedByUI(Key key) const {
		const bool pointer = static_cast<i32>(key) >= 10001;
		return (pointer ? _uiPointer || _editorPointer : _uiKeyboard || _editorKeyboard) ||
			   _editorKeys.contains(static_cast<i32>(key));
	}

	// True when the window mouse lies inside the present rect.
	bool IsMouseOverViewport() const;

	Vector2 ScrollDelta() const {
		return (_uiPointer || _editorPointer) ? Vector2{} : _scrollDelta;
	}

	// Pushed each frame by Application after computing the present rect.
	void SetViewportMapping(const Rect &presentRect, Vector2 contentSize, Vector2 framebufferSize,
		Vector2 viewportSize);

	EventCallbackList &Events() {
		return _events;
	}

	CharCallbackList &CharEvents() {
		return _charEvents;
	}

	// Raw window events before batching or capture filtering, used by binding capture.
	// Gameplay receives filtered Events().
	EventCallbackList &UnfilteredEvents() {
		return _unfilteredEvents;
	}

	// Replaces every action. Application loads these from project.sowa.
	void SetActions(const std::vector<InputAction> &actions);
	void SetActionBindings(std::string_view name, std::vector<InputBinding> bindings);
	const std::vector<InputBinding> &GetActionBindings(std::string_view name) const;

	// Pressed while any uncaptured binding and its required modifiers are held.
	// Edges apply to the action as a whole, not to individual bindings.
	bool IsActionPressed(std::string_view name) const;
	bool IsActionJustPressed(std::string_view name) const;
	bool IsActionJustReleased(std::string_view name) const;
	// Action strength from 0 to 1. Keys and mouse buttons return exactly 0 or 1.
	f32 GetActionStrength(std::string_view name) const;
	f32 GetAxis(std::string_view negative, std::string_view positive) const;
	Vector2 GetVector(std::string_view negativeX, std::string_view positiveX,
		std::string_view negativeY, std::string_view positiveY) const;

	// Called by Window (and by tests). Repeat updates events only - not poll state.
	void HandleKey(Key key, KeyAction action, i32 mods = 0);
	void HandleMouseButton(i32 button, KeyAction action, i32 mods = 0);
	void HandleCursorPos(f32 x, f32 y);
	void HandleScroll(f32 xOffset, f32 yOffset);
	void HandleChar(u32 codepoint);

  private:
	enum class KeyState {
		Up,
		Down,
		JustPressed,
		JustReleased,
	};

	void emit(const InputEvent &event);
	void dispatch(const InputEvent &event);
	KeyState stateOf(Key key) const;
	// previous asks about last frame, recovered from this frame's edges.
	bool isHeld(Key key, bool previous) const;
	bool isBindingActive(const InputBinding &binding, bool previous) const;
	bool isActionActive(std::string_view name, bool previous) const;
	void setKeyState(Key key, KeyAction action);

	std::unordered_map<i32, KeyState> _keyStates;
	std::unordered_map<std::string, std::vector<InputBinding>> _actions;

	EventCallbackList _events;
	EventCallbackList _unfilteredEvents;
	CharCallbackList _charEvents;

	Vector2 _cursorPos;
	Vector2 _scrollDelta;

	Rect _presentRect;
	Vector2 _contentSize{1.f, 1.f};
	Vector2 _framebufferSize{1.f, 1.f};
	Vector2 _viewportSize{1.f, 1.f};
	bool _hasViewportMapping = false;
	bool _uiPointer = false, _uiKeyboard = false;
	bool _editorPointer = false, _editorKeyboard = false;
	bool _batchEvents = false;
	std::vector<InputEvent> _pendingEvents;
	std::unordered_set<i32> _editorKeys;
};

} // namespace sowa
