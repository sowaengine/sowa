#include <doctest/doctest.h>

#include "core/input.hpp"
#include "editor/binding_capture.hpp"

using namespace sowa;
using State = editor::BindingCapture::State;

namespace {

InputEvent keyEvent(Key key, KeyAction action, i32 mods = 0) {
	InputEvent event;
	event.type = InputEventType::Key;
	event.key.key = key;
	event.key.action = action;
	event.key.mods = mods;
	return event;
}

InputEvent mouseEvent(Key button, KeyAction action, i32 mods = 0) {
	InputEvent event;
	event.type = InputEventType::MouseButton;
	event.button.button = button;
	event.button.action = action;
	event.button.mods = mods;
	return event;
}

} // namespace

TEST_CASE("Binding capture ignores events until it begins") {
	editor::BindingCapture capture;
	capture.Feed(keyEvent(Key::S, KeyAction::Press));
	CHECK(capture.GetState() == State::Idle);
	CHECK_FALSE(capture.Take());
}

TEST_CASE("Binding capture records a key with the modifiers held") {
	editor::BindingCapture capture;
	capture.Begin();
	capture.Feed(keyEvent(Key::LeftControl, KeyAction::Press, InputModifierControl));
	CHECK(capture.IsListening());
	capture.Feed(keyEvent(Key::S, KeyAction::Press, InputModifierControl | InputModifierShift));
	REQUIRE(capture.GetState() == State::Captured);

	const auto binding = capture.Take();
	REQUIRE(binding);
	CHECK(*binding == InputBinding{Key::S, InputModifierControl | InputModifierShift});
	CHECK(capture.GetState() == State::Idle);

	// Later events after completion are ignored.
	capture.Feed(keyEvent(Key::LeftControl, KeyAction::Release));
	CHECK(capture.GetState() == State::Idle);
}

TEST_CASE("Binding capture records a lone modifier on release") {
	editor::BindingCapture capture;
	capture.Begin();
	capture.Feed(keyEvent(Key::LeftShift, KeyAction::Press, InputModifierShift));
	CHECK(capture.IsListening());
	capture.Feed(keyEvent(Key::LeftShift, KeyAction::Release));
	const auto binding = capture.Take();
	REQUIRE(binding);
	CHECK(*binding == InputBinding{Key::LeftShift, 0});
}

TEST_CASE("Binding capture ignores the release of the click that started it") {
	editor::BindingCapture capture;
	capture.Begin();
	capture.Feed(mouseEvent(Key::MouseLeft, KeyAction::Release));
	capture.Feed(keyEvent(Key::LeftAlt, KeyAction::Release));
	capture.Feed(keyEvent(Key::A, KeyAction::Repeat));
	CHECK(capture.IsListening());

	capture.Feed(mouseEvent(Key::MouseRight, KeyAction::Press, InputModifierAlt));
	const auto binding = capture.Take();
	REQUIRE(binding);
	CHECK(*binding == InputBinding{Key::MouseRight, InputModifierAlt});
}

TEST_CASE("Escape is recorded like any other key") {
	editor::BindingCapture capture;
	capture.Begin();
	capture.Feed(keyEvent(Key::Escape, KeyAction::Press));
	const auto binding = capture.Take();
	REQUIRE(binding);
	CHECK(*binding == InputBinding{Key::Escape, 0});
}

TEST_CASE("Binding capture skips keys that have no name") {
	editor::BindingCapture capture;
	capture.Begin();
	capture.Feed(keyEvent(Key::Unknown, KeyAction::Press));
	CHECK(capture.IsListening());
	capture.Cancel();
	CHECK(capture.GetState() == State::Cancelled);
}
