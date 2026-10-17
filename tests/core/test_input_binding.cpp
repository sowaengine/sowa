#include <doctest/doctest.h>

#include <optional>
#include <string>

#include "core/input_binding.hpp"

using namespace sowa;

TEST_CASE("Input bindings parse and format") {
	const std::optional<InputBinding> plain = ParseInputBinding("Space");
	REQUIRE(plain);
	CHECK(plain->key == Key::Space);
	CHECK(plain->modifiers == 0);

	const std::optional<InputBinding> chord = ParseInputBinding("Shift+Ctrl+S");
	REQUIRE(chord);
	CHECK(chord->key == Key::S);
	CHECK(chord->modifiers == (InputModifierControl | InputModifierShift));
	// Canonical modifier order on the way out.
	CHECK(FormatInputBinding(*chord) == "Ctrl+Shift+S");

	CHECK(FormatInputBinding({Key::MouseLeft, InputModifierAlt | InputModifierSuper}) ==
		  "Alt+Super+MouseLeft");
	CHECK(FormatInputBinding({Key::Unknown, 0}).empty());
}

TEST_CASE("Aliased keys parse and format under the preferred name") {
	const std::optional<InputBinding> alias = ParseInputBinding("Mouse1");
	REQUIRE(alias);
	CHECK(alias->key == Key::MouseLeft);
	CHECK(FormatInputBinding(*alias) == "MouseLeft");
}

TEST_CASE("Every named key round-trips through binding text") {
	for (const EnumConstant &constant : KeyConstants()) {
		const InputBinding binding{static_cast<Key>(constant.value), InputModifierControl};
		const std::optional<InputBinding> parsed = ParseInputBinding(FormatInputBinding(binding));
		REQUIRE_MESSAGE(parsed, constant.name);
		CHECK(*parsed == binding);
	}
}

TEST_CASE("Malformed binding text is rejected") {
	CHECK_FALSE(ParseInputBinding(""));
	CHECK_FALSE(ParseInputBinding("space"));
	CHECK_FALSE(ParseInputBinding("Ctrl+"));
	CHECK_FALSE(ParseInputBinding("Ctrl"));
	CHECK_FALSE(ParseInputBinding("Ctrl+Ctrl+S"));
	CHECK_FALSE(ParseInputBinding("Hyper+S"));
	CHECK_FALSE(ParseInputBinding("Unknown"));
	CHECK_FALSE(ParseInputBinding("S+Ctrl"));
}

TEST_CASE("Modifier keys report their modifier bit") {
	CHECK(ModifierOfKey(Key::LeftControl) == InputModifierControl);
	CHECK(ModifierOfKey(Key::RightShift) == InputModifierShift);
	CHECK(ModifierOfKey(Key::LeftAlt) == InputModifierAlt);
	CHECK(ModifierOfKey(Key::RightSuper) == InputModifierSuper);
	CHECK(ModifierOfKey(Key::S) == 0);
}
