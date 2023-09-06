#include "input.hxx"

#include "servers/input_server.hxx"

void Input::GetMousePosition(double &x, double &y) {
	return InputServer::GetInstance().GetMousePosition(x, y);
}
void Input::GetWindowMousePosition(double &x, double &y) {
	return InputServer::GetInstance().GetWindowMousePosition(x, y);
}
int Input::GetPressedChar() {
	return InputServer::GetInstance().GetPressedChar();
}

ActionState Input::GetKeyState(int key) {
	return InputServer::GetInstance().GetKeyState(key);
}
bool Input::IsKeyDown(int key) {
	return InputServer::GetInstance().IsKeyDown(key);
}
bool Input::IsKeyUp(int key) {
	return InputServer::GetInstance().IsKeyUp(key);
}
bool Input::IsKeyJustPressed(int key) {
	return InputServer::GetInstance().IsKeyJustPressed(key);
}
bool Input::IsKeyJustReleased(int key) {
	return InputServer::GetInstance().IsKeyJustReleased(key);
}

ActionState Input::GetButtonState(int button) {
	return InputServer::GetInstance().GetButtonState(button);
}
bool Input::IsButtonDown(int button) {
	return InputServer::GetInstance().IsButtonDown(button);
}
bool Input::IsButtonUp(int button) {
	return InputServer::GetInstance().IsButtonUp(button);
}
bool Input::IsButtonJustPressed(int button) {
	return InputServer::GetInstance().IsButtonJustPressed(button);
}
bool Input::IsButtonJustReleased(int button) {
	return InputServer::GetInstance().IsButtonJustReleased(button);
}