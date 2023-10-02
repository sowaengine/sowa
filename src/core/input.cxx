#include "input.hxx"

#include "servers/input_server.hxx"

void Input::GetMousePosition(double &x, double &y) {
	return InputServer::get().GetMousePosition(x, y);
}
void Input::GetWindowMousePosition(double &x, double &y) {
	return InputServer::get().GetWindowMousePosition(x, y);
}
vec2 Input::GetMousePosition() {
	double x, y;
	InputServer::get().GetMousePosition(x, y);
	vec2 v(x, y);
	return v;
}
vec2 Input::GetWindowMousePosition() {
	double x, y;
	InputServer::get().GetWindowMousePosition(x, y);
	vec2 v(x, y);
	return v;
}
int Input::GetPressedChar() {
	return InputServer::get().GetPressedChar();
}

ActionState Input::GetKeyState(int key) {
	return InputServer::get().GetKeyState(key);
}
bool Input::IsKeyDown(int key) {
	return InputServer::get().IsKeyDown(key);
}
bool Input::IsKeyUp(int key) {
	return InputServer::get().IsKeyUp(key);
}
bool Input::IsKeyJustPressed(int key) {
	return InputServer::get().IsKeyJustPressed(key);
}
bool Input::IsKeyJustReleased(int key) {
	return InputServer::get().IsKeyJustReleased(key);
}

ActionState Input::GetButtonState(int button) {
	return InputServer::get().GetButtonState(button);
}
bool Input::IsButtonDown(int button) {
	return InputServer::get().IsButtonDown(button);
}
bool Input::IsButtonUp(int button) {
	return InputServer::get().IsButtonUp(button);
}
bool Input::IsButtonJustPressed(int button) {
	return InputServer::get().IsButtonJustPressed(button);
}
bool Input::IsButtonJustReleased(int button) {
	return InputServer::get().IsButtonJustReleased(button);
}
bool Input::IsButtonJustClicked(int button) {
	return InputServer::get().IsButtonJustClicked(button);
}
bool Input::IsButtonJustDoubleClicked(int button) {
	return InputServer::get().IsButtonJustDoubleClicked(button);
}
vec2 Input::GetMouseMotion() {
	return InputServer::get().GetMouseMotion();
}
bool Input::IsCursorInside() {
	return InputServer::get().IsCursorInside();
}