#include <doctest/doctest.h>

#include <vector>

#include "core/input.hpp"
#include "core/input_binding.hpp"

using namespace sowa;

TEST_CASE("Input key press/release edge states ignore Repeat") {
	Input input;
	int keyEvents = 0;
	KeyAction lastAction = KeyAction::Release;
	input.Events().append([&](const InputEvent &e) {
		if (e.type == InputEventType::Key) {
			keyEvents++;
			lastAction = e.key.action;
		}
	});

	input.HandleKey(Key::A, KeyAction::Press);
	CHECK(input.IsKeyJustPressed(Key::A));
	CHECK(input.IsKeyDown(Key::A));
	CHECK(keyEvents == 1);

	input.HandleKey(Key::A, KeyAction::Repeat);
	CHECK(input.IsKeyJustPressed(Key::A)); // poll unchanged
	CHECK(input.IsKeyDown(Key::A));
	CHECK(keyEvents == 2);
	CHECK(lastAction == KeyAction::Repeat);

	input.Update();
	CHECK_FALSE(input.IsKeyJustPressed(Key::A));
	CHECK(input.IsKeyDown(Key::A));

	input.HandleKey(Key::A, KeyAction::Release);
	CHECK(input.IsKeyJustReleased(Key::A));
	CHECK_FALSE(input.IsKeyDown(Key::A));

	input.Update();
	CHECK_FALSE(input.IsKeyJustReleased(Key::A));
	CHECK_FALSE(input.IsKeyDown(Key::A));
}

TEST_CASE("Input mouse buttons map into Key range") {
	Input input;
	input.HandleMouseButton(0, KeyAction::Press);
	CHECK(input.IsKeyJustPressed(Key::MouseLeft));
	input.HandleMouseButton(1, KeyAction::Press);
	CHECK(input.IsKeyJustPressed(Key::MouseRight));
}

TEST_CASE("Input cursor and scroll") {
	Input input;
	input.HandleCursorPos(10.f, 20.f);
	CHECK(input.GetWindowMousePosition().x == doctest::Approx(10.f));
	CHECK(input.GetWindowMousePosition().y == doctest::Approx(20.f));
	// Without a viewport mapping, GetMousePosition falls back to window coords.
	CHECK(input.GetMousePosition().x == doctest::Approx(10.f));
	CHECK(input.GetMousePosition().y == doctest::Approx(20.f));

	f32 deltaX = 0.f;
	input.Events().append([&](const InputEvent &e) {
		if (e.type == InputEventType::MouseMove) {
			deltaX = e.mouseMove.deltaX;
		}
	});
	input.HandleCursorPos(13.f, 20.f);
	CHECK(deltaX == doctest::Approx(-3.f)); // oldPos - newPos

	input.HandleScroll(1.5f, -2.f);
	CHECK(input.ScrollDelta().x == doctest::Approx(1.5f));
	CHECK(input.ScrollDelta().y == doctest::Approx(-2.f));
	input.Update();
	CHECK(input.ScrollDelta().x == doctest::Approx(0.f));
	CHECK(input.ScrollDelta().y == doctest::Approx(0.f));
}

TEST_CASE("Input maps window mouse into viewport space") {
	Input input;
	// Content == framebuffer (no retina), Stretch present fills the window.
	const Rect present(0.f, 0.f, 800.f, 600.f);
	input.SetViewportMapping(present, Vector2(800.f, 600.f),
							 Vector2(800.f, 600.f), Vector2(1920.f, 1080.f));

	// Top-left of the window (Y-down) → bottom-left of the viewport (Y-up).
	input.HandleCursorPos(0.f, 600.f);
	CHECK(input.IsMouseOverViewport());
	CHECK(input.GetMousePosition().x == doctest::Approx(0.f));
	CHECK(input.GetMousePosition().y == doctest::Approx(0.f));

	// Center of the window.
	input.HandleCursorPos(400.f, 300.f);
	CHECK(input.GetMousePosition().x == doctest::Approx(960.f));
	CHECK(input.GetMousePosition().y == doctest::Approx(540.f));

	// Outside present rect (letterbox case).
	input.SetViewportMapping(Rect(100.f, 0.f, 600.f, 600.f),
							 Vector2(800.f, 600.f), Vector2(800.f, 600.f),
							 Vector2(100.f, 100.f));
	input.HandleCursorPos(50.f, 300.f);
	CHECK_FALSE(input.IsMouseOverViewport());
}

TEST_CASE("Input CharEvents and Char InputEvent") {
	Input input;
	std::vector<u32> chars;
	int charEvents = 0;
	input.CharEvents().append([&](u32 codepoint) { chars.push_back(codepoint); });
	input.Events().append([&](const InputEvent &e) {
		if (e.type == InputEventType::Char) {
			charEvents++;
			CHECK(e.character.codepoint == 'a');
		}
	});

	input.HandleChar(static_cast<u32>('a'));
	CHECK(chars.size() == 1);
	CHECK(chars[0] == static_cast<u32>('a'));
	CHECK(charEvents == 1);

	// OS-level key repeat delivers more char callbacks - same path.
	input.HandleChar(static_cast<u32>('a'));
	CHECK(chars.size() == 2);
}

TEST_CASE("Input action mapping") {
	Input input;
	input.SetActionBindings("left", {{Key::A}, {Key::Left}});
	input.SetActionBindings("right", {{Key::D}, {Key::Right}});
	input.SetActionBindings("up", {{Key::W}});
	input.SetActionBindings("down", {{Key::S}});

	CHECK(input.GetActionBindings("left").size() == 2);
	CHECK(input.GetActionBindings("missing").empty());

	input.HandleKey(Key::A, KeyAction::Press);
	CHECK(input.IsActionJustPressed("left"));
	CHECK(input.IsActionPressed("left"));
	CHECK(input.GetAxis("left", "right") == doctest::Approx(-1.f));

	input.Update();
	input.HandleKey(Key::A, KeyAction::Release);
	input.HandleKey(Key::D, KeyAction::Press);
	CHECK(input.GetAxis("left", "right") == doctest::Approx(1.f));

	input.HandleKey(Key::W, KeyAction::Press);
	Vector2 axis = input.GetVector("left", "right", "down", "up");
	CHECK(axis.x == doctest::Approx(1.f));
	CHECK(axis.y == doctest::Approx(1.f));
}

// Input edges must remain visible until all frame consumers have read them.
TEST_CASE("Edges survive the whole frame when Update runs before polling") {
	Input input;

	auto pollEvents = [&] { input.HandleKey(Key::Space, KeyAction::Press); };

	// Frame 1: age previous edges, then poll, then run game code.
	input.Update();
	pollEvents();
	CHECK(input.IsKeyJustPressed(Key::Space)); // game code sees the press
	CHECK(input.IsKeyDown(Key::Space));

	// Frame 2: no new events; the edge is retired but the key stays held.
	input.Update();
	CHECK_FALSE(input.IsKeyJustPressed(Key::Space));
	CHECK(input.IsKeyDown(Key::Space));
}

TEST_CASE("Polling before Update consumes the edge in the same frame") {
	Input input;

	// The broken order, pinned down so the difference is explicit.
	input.HandleKey(Key::Space, KeyAction::Press);
	input.Update();
	CHECK_FALSE(input.IsKeyJustPressed(Key::Space));
	CHECK(input.IsKeyDown(Key::Space));
}

TEST_CASE("Scroll delta survives the frame it arrives in") {
	Input input;

	input.Update();
	input.HandleScroll(0.f, 3.f);
	CHECK(input.ScrollDelta().y == doctest::Approx(3.f));

	input.Update();
	CHECK(input.ScrollDelta().y == doctest::Approx(0.f));
}

TEST_CASE("Editor capture blocks polling, actions and both input event streams") {
	Input input;
	int keys = 0, chars = 0, events = 0;
	input.Events().append([&](const InputEvent &event) {
		++events;
		if (event.type == InputEventType::Key) ++keys;
	});
	input.CharEvents().append([&](u32) { ++chars; });
	input.SetActionBindings("move", {{Key::A}});
	input.BeginEventBatch();
	input.HandleKey(Key::A, KeyAction::Press);
	input.HandleChar('a');
	CHECK(events == 0);
	input.SetEditorCapture(false, true);
	input.DispatchEvents();
	CHECK(keys == 0);
	CHECK(chars == 0);
	CHECK(events == 0);
	CHECK_FALSE(input.IsKeyDown(Key::A));
	CHECK_FALSE(input.IsActionJustPressed("move"));

	// Game UI must not be able to clear the editor's independent capture.
	input.SetUICapture(false, false);
	CHECK_FALSE(input.IsActionPressed("move"));
	input.Update();
	input.SetEditorCapture(false, false);
	CHECK_FALSE(input.IsKeyDown(Key::A));
	input.HandleKey(Key::A, KeyAction::Release);
	CHECK_FALSE(input.IsKeyJustReleased(Key::A));
	CHECK(events == 0);
	input.Update();
	input.HandleKey(Key::A, KeyAction::Press);
	input.HandleChar('a');
	CHECK(input.IsActionJustPressed("move"));
	CHECK(keys == 1);
	CHECK(chars == 1);
}

TEST_CASE("A held game key survives a click elsewhere in the editor") {
	Input input;
	input.SetActionBindings("move", {{Key::D}});
	input.HandleKey(Key::D, KeyAction::Press);
	input.Update();
	REQUIRE(input.IsKeyDown(Key::D));

	// A click on an editor button turns keyboard capture on for a frame.
	input.SetEditorCapture(true, true);
	CHECK_FALSE(input.IsKeyDown(Key::D));
	input.Update();

	input.SetEditorCapture(false, false);
	CHECK(input.IsKeyDown(Key::D));
	CHECK(input.IsActionPressed("move"));
	CHECK_FALSE(input.IsActionJustPressed("move"));
}

TEST_CASE("Editor pointer capture blocks clicks and scroll without swallowing viewport reentry") {
	Input input;
	input.SetEditorCapture(true, false);
	input.HandleMouseButton(0, KeyAction::Press);
	input.HandleScroll(0, 1);
	CHECK_FALSE(input.IsKeyJustPressed(Key::MouseLeft));
	CHECK(input.ScrollDelta() == Vector2{});
	input.HandleMouseButton(0, KeyAction::Release);
	input.Update();

	// Poll moves into the viewport before this frame's editor capture is resolved.
	input.BeginEventBatch();
	input.HandleMouseButton(0, KeyAction::Press);
	input.SetEditorCapture(false, false);
	input.DispatchEvents();
	CHECK(input.IsKeyJustPressed(Key::MouseLeft));
}

TEST_CASE("An action fires from any of its bindings, with edges on the action") {
	Input input;
	input.SetActions({{"jump", {{Key::Space}, {Key::W}, {Key::MouseLeft}}}});

	input.HandleKey(Key::W, KeyAction::Press);
	CHECK(input.IsActionJustPressed("jump"));
	CHECK(input.GetActionStrength("jump") == doctest::Approx(1.f));

	// A second binding while the first is held is not a new press.
	input.Update();
	input.HandleKey(Key::Space, KeyAction::Press);
	CHECK(input.IsActionPressed("jump"));
	CHECK_FALSE(input.IsActionJustPressed("jump"));

	// Releasing one of two held bindings keeps the action pressed.
	input.Update();
	input.HandleKey(Key::W, KeyAction::Release);
	CHECK(input.IsActionPressed("jump"));
	CHECK_FALSE(input.IsActionJustReleased("jump"));

	input.Update();
	input.HandleKey(Key::Space, KeyAction::Release);
	CHECK_FALSE(input.IsActionPressed("jump"));
	CHECK(input.IsActionJustReleased("jump"));
	CHECK(input.GetActionStrength("jump") == doctest::Approx(0.f));

	input.Update();
	input.HandleMouseButton(0, KeyAction::Press);
	CHECK(input.IsActionJustPressed("jump"));
}

TEST_CASE("SetActions replaces every action") {
	Input input;
	input.SetActionBindings("old", {{Key::A}});
	input.SetActions({{"new", {{Key::B}}}});
	CHECK(input.GetActionBindings("old").empty());
	REQUIRE(input.GetActionBindings("new").size() == 1);
	CHECK(input.GetActionBindings("new")[0].key == Key::B);
}

TEST_CASE("A binding with modifiers needs them held") {
	Input input;
	input.SetActions({{"save", {{Key::S, InputModifierControl}}}, {"down", {{Key::S}}}});

	input.HandleKey(Key::S, KeyAction::Press);
	CHECK_FALSE(input.IsActionPressed("save"));
	CHECK(input.IsActionPressed("down"));

	// Holding the modifier after the key starts the action on that frame.
	input.Update();
	input.HandleKey(Key::LeftControl, KeyAction::Press);
	CHECK(input.IsActionJustPressed("save"));
	// Plain bindings ignore extra modifiers.
	CHECK(input.IsActionPressed("down"));
	CHECK_FALSE(input.IsActionJustPressed("down"));

	// Letting go of the modifier releases the action.
	input.Update();
	input.HandleKey(Key::LeftControl, KeyAction::Release);
	CHECK(input.IsActionJustReleased("save"));
	CHECK(input.IsActionPressed("down"));
}

TEST_CASE("Either side's modifier key satisfies a binding") {
	Input input;
	input.SetActions({{"run", {{Key::W, InputModifierShift | InputModifierAlt}}}});

	input.HandleKey(Key::RightShift, KeyAction::Press);
	input.HandleKey(Key::W, KeyAction::Press);
	CHECK_FALSE(input.IsActionPressed("run"));
	input.HandleKey(Key::RightAlt, KeyAction::Press);
	CHECK(input.IsActionPressed("run"));

	input.HandleKey(Key::RightShift, KeyAction::Release);
	input.HandleKey(Key::LeftShift, KeyAction::Press);
	CHECK(input.IsActionPressed("run"));
}

TEST_CASE("UI capture blocks action bindings") {
	Input input;
	input.SetActions({{"fire", {{Key::MouseLeft}}}, {"save", {{Key::S, InputModifierControl}}}});
	input.HandleMouseButton(0, KeyAction::Press);
	input.HandleKey(Key::LeftControl, KeyAction::Press);
	input.HandleKey(Key::S, KeyAction::Press);

	input.SetUICapture(true, true);
	CHECK_FALSE(input.IsActionPressed("fire"));
	CHECK_FALSE(input.IsActionJustPressed("fire"));
	CHECK_FALSE(input.IsActionPressed("save"));

	input.SetUICapture(false, false);
	CHECK(input.IsActionJustPressed("fire"));
	CHECK(input.IsActionJustPressed("save"));
}
