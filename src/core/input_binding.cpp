#include "core/input_binding.hpp"

#include <array>

namespace sowa {
namespace {

EnumConstant key(const char *name, Key value) {
	return {name, static_cast<i32>(value)};
}

struct ModifierName {
	u8 bit;
	std::string_view name;
};

// Also the order FormatInputBinding writes them in.
constexpr std::array<ModifierName, 4> kModifierNames{{
	{InputModifierControl, "Ctrl"},
	{InputModifierShift, "Shift"},
	{InputModifierAlt, "Alt"},
	{InputModifierSuper, "Super"},
}};

} // namespace

const std::vector<EnumConstant> &KeyConstants() {
	static const std::vector<EnumConstant> constants{
		key("Space", Key::Space),
		key("Apostrophe", Key::Apostrophe),
		key("Comma", Key::Comma),
		key("Minus", Key::Minus),
		key("Period", Key::Period),
		key("Slash", Key::Slash),
		key("Zero", Key::Zero),
		key("One", Key::One),
		key("Two", Key::Two),
		key("Three", Key::Three),
		key("Four", Key::Four),
		key("Five", Key::Five),
		key("Six", Key::Six),
		key("Seven", Key::Seven),
		key("Eight", Key::Eight),
		key("Nine", Key::Nine),
		key("Semicolon", Key::Semicolon),
		key("Equal", Key::Equal),
		key("A", Key::A),
		key("B", Key::B),
		key("C", Key::C),
		key("D", Key::D),
		key("E", Key::E),
		key("F", Key::F),
		key("G", Key::G),
		key("H", Key::H),
		key("I", Key::I),
		key("J", Key::J),
		key("K", Key::K),
		key("L", Key::L),
		key("M", Key::M),
		key("N", Key::N),
		key("O", Key::O),
		key("P", Key::P),
		key("Q", Key::Q),
		key("R", Key::R),
		key("S", Key::S),
		key("T", Key::T),
		key("U", Key::U),
		key("V", Key::V),
		key("W", Key::W),
		key("X", Key::X),
		key("Y", Key::Y),
		key("Z", Key::Z),
		key("LeftBracket", Key::LeftBracket),
		key("Backslash", Key::Backslash),
		key("RightBracket", Key::RightBracket),
		key("GraveAccent", Key::GraveAccent),
		key("World1", Key::World1),
		key("World2", Key::World2),
		key("Escape", Key::Escape),
		key("Enter", Key::Enter),
		key("Tab", Key::Tab),
		key("Backspace", Key::Backspace),
		key("Insert", Key::Insert),
		key("Delete", Key::Delete),
		key("Right", Key::Right),
		key("Left", Key::Left),
		key("Down", Key::Down),
		key("Up", Key::Up),
		key("PageUp", Key::PageUp),
		key("PageDown", Key::PageDown),
		key("Home", Key::Home),
		key("End", Key::End),
		key("CapsLock", Key::CapsLock),
		key("ScrollLock", Key::ScrollLock),
		key("NumLock", Key::NumLock),
		key("PrintScreen", Key::PrintScreen),
		key("Pause", Key::Pause),
		key("F1", Key::F1),
		key("F2", Key::F2),
		key("F3", Key::F3),
		key("F4", Key::F4),
		key("F5", Key::F5),
		key("F6", Key::F6),
		key("F7", Key::F7),
		key("F8", Key::F8),
		key("F9", Key::F9),
		key("F10", Key::F10),
		key("F11", Key::F11),
		key("F12", Key::F12),
		key("F13", Key::F13),
		key("F14", Key::F14),
		key("F15", Key::F15),
		key("F16", Key::F16),
		key("F17", Key::F17),
		key("F18", Key::F18),
		key("F19", Key::F19),
		key("F20", Key::F20),
		key("F21", Key::F21),
		key("F22", Key::F22),
		key("F23", Key::F23),
		key("F24", Key::F24),
		key("F25", Key::F25),
		key("Keypad0", Key::Keypad0),
		key("Keypad1", Key::Keypad1),
		key("Keypad2", Key::Keypad2),
		key("Keypad3", Key::Keypad3),
		key("Keypad4", Key::Keypad4),
		key("Keypad5", Key::Keypad5),
		key("Keypad6", Key::Keypad6),
		key("Keypad7", Key::Keypad7),
		key("Keypad8", Key::Keypad8),
		key("Keypad9", Key::Keypad9),
		key("KeypadDecimal", Key::KeypadDecimal),
		key("KeypadDivide", Key::KeypadDivide),
		key("KeypadMultiply", Key::KeypadMultiply),
		key("KeypadSubtract", Key::KeypadSubtract),
		key("KeypadAdd", Key::KeypadAdd),
		key("KeypadEnter", Key::KeypadEnter),
		key("KeypadEqual", Key::KeypadEqual),
		key("LeftShift", Key::LeftShift),
		key("LeftControl", Key::LeftControl),
		key("LeftAlt", Key::LeftAlt),
		key("LeftSuper", Key::LeftSuper),
		key("RightShift", Key::RightShift),
		key("RightControl", Key::RightControl),
		key("RightAlt", Key::RightAlt),
		key("RightSuper", Key::RightSuper),
		key("Menu", Key::Menu),
		key("MouseLeft", Key::MouseLeft),
		key("MouseRight", Key::MouseRight),
		key("MouseMiddle", Key::MouseMiddle),
		key("Mouse1", Key::Mouse1),
		key("Mouse2", Key::Mouse2),
		key("Mouse3", Key::Mouse3),
		key("Mouse4", Key::Mouse4),
		key("Mouse5", Key::Mouse5),
		key("Mouse6", Key::Mouse6),
		key("Mouse7", Key::Mouse7),
		key("Mouse8", Key::Mouse8),
	};
	return constants;
}

std::string_view KeyName(Key key) {
	for (const EnumConstant &constant : KeyConstants()) {
		if (constant.value == static_cast<i32>(key)) {
			return constant.name;
		}
	}
	return {};
}

std::optional<Key> KeyFromName(std::string_view name) {
	for (const EnumConstant &constant : KeyConstants()) {
		if (constant.name == name) {
			return static_cast<Key>(constant.value);
		}
	}
	return std::nullopt;
}

u8 ModifierOfKey(Key key) {
	switch (key) {
	case Key::LeftShift:
	case Key::RightShift:
		return InputModifierShift;
	case Key::LeftControl:
	case Key::RightControl:
		return InputModifierControl;
	case Key::LeftAlt:
	case Key::RightAlt:
		return InputModifierAlt;
	case Key::LeftSuper:
	case Key::RightSuper:
		return InputModifierSuper;
	default:
		return 0;
	}
}

std::optional<InputBinding> ParseInputBinding(std::string_view text) {
	InputBinding binding;
	while (true) {
		const std::size_t plus = text.find('+');
		if (plus == std::string_view::npos) {
			break;
		}
		const std::string_view part = text.substr(0, plus);
		text.remove_prefix(plus + 1);

		u8 bit = 0;
		for (const ModifierName &modifier : kModifierNames) {
			if (modifier.name == part) {
				bit = modifier.bit;
			}
		}
		if (bit == 0 || (binding.modifiers & bit) != 0) {
			return std::nullopt;
		}
		binding.modifiers |= bit;
	}

	const std::optional<Key> key = KeyFromName(text);
	if (!key) {
		return std::nullopt;
	}
	binding.key = *key;
	return binding;
}

std::string FormatInputBinding(const InputBinding &binding) {
	const std::string_view name = KeyName(binding.key);
	if (name.empty()) {
		return {};
	}
	std::string text;
	for (const ModifierName &modifier : kModifierNames) {
		if ((binding.modifiers & modifier.bit) != 0) {
			text += modifier.name;
			text += '+';
		}
	}
	text += name;
	return text;
}

} // namespace sowa
