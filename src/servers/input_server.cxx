#include "input_server.hxx"
#include "core/app.hxx"
#include "core/input.hxx"
#include "rendering_server.hxx"

struct InputCallbackBridge {
	InputCallbackBridge() = default;

	void MouseButtonCallback(GLFWwindow *window, int button, int action, int mods) {
		InputServer::GetInstance().mouse_button_callback(window, button, action, mods);
	}

	void CursorPosCalllback(GLFWwindow *window, double x, double y) {
		InputServer::GetInstance().cursor_pos_callback(window, x, y);
	}

	void KeyCallback(GLFWwindow *window, int key, int scancode, int action, int mods) {
		InputServer::GetInstance().key_callback(window, key, scancode, action, mods);
	}

	void CharCallback(GLFWwindow *window, unsigned int codepoint) {
		InputServer::GetInstance().char_callback(window, codepoint);
	}
};

static void CallbackWrapperMouseButtonCallback(GLFWwindow *window, int button, int action, int mods) {
	InputCallbackBridge().MouseButtonCallback(window, button, action, mods);
}

static void CallbackWrapperCursorPosCallback(GLFWwindow *window, double x, double y) {
	InputCallbackBridge().CursorPosCalllback(window, x, y);
}

static void CallbackWrapperKeyCallback(GLFWwindow *window, int key, int scancode, int action, int mods) {
	InputCallbackBridge().KeyCallback(window, key, scancode, action, mods);
}

static void CallbackWrapperCharCallback(GLFWwindow *window, unsigned int codepoint) {
	InputCallbackBridge().CharCallback(window, codepoint);
}

InputServer::InputServer() {
}

InputServer &InputServer::GetInstance() {
	static InputServer *server = new InputServer;
	return *server;
}

void InputServer::ProcessInput() {
}

void InputServer::PollEvents() {
	//
	m_pressedChar = 0;

	for (auto &[key, action] : m_keyStates) {
		if (action == ActionState::JUST_PRESSED)
			action = ActionState::DOWN;
		else if (action == ActionState::JUST_RELEASED)
			action = ActionState::UP;
	}

	for (auto &[button, action] : m_buttonStates) {
		if (action == ActionState::JUST_PRESSED)
			action = ActionState::DOWN;
		else if (action == ActionState::JUST_RELEASED)
			action = ActionState::UP;
	}

	//
	glfwPollEvents();
}

void InputServer::GetMousePosition(double &x, double &y) {
	int w, h;
	RenderingServer::GetInstance().GetWindowSize(w, h);

	x = m_input_mouseX;
	y = h - m_input_mouseY;
}
void InputServer::GetWindowMousePosition(double &x, double &y) {
	x = m_input_mouseX;
	y = m_input_mouseY;
}

int InputServer::GetPressedChar() {
	return m_pressedChar;
}

//-------------------------------------------------------//
//--------------------- Key Input -----------------------//
//-------------------------------------------------------//

ActionState InputServer::GetKeyState(int key) {
	return m_keyStates[key];
}

bool InputServer::IsKeyDown(int key) {
	ActionState state = GetKeyState(key);
	return state == ActionState::JUST_PRESSED || state == ActionState::DOWN;
}

bool InputServer::IsKeyUp(int key) {
	ActionState state = GetKeyState(key);
	return state == ActionState::JUST_RELEASED || state == ActionState::UP;
}

bool InputServer::IsKeyJustPressed(int key) {
	return GetKeyState(key) == ActionState::JUST_PRESSED;
}

bool InputServer::IsKeyJustReleased(int key) {
	return GetKeyState(key) == ActionState::JUST_RELEASED;
}

ActionState InputServer::GetButtonState(int button) {
	return m_buttonStates[button];
}

//-------------------------------------------------------//
//------------------ Button Input -----------------------//
//-------------------------------------------------------//

bool InputServer::IsButtonDown(int button) {
	ActionState state = GetButtonState(button);
	return state == ActionState::JUST_PRESSED && state == ActionState::DOWN;
}

bool InputServer::IsButtonUp(int button) {
	ActionState state = GetButtonState(button);
	return state == ActionState::JUST_RELEASED && state == ActionState::UP;
}

bool InputServer::IsButtonJustPressed(int button) {
	return GetButtonState(button) == ActionState::JUST_PRESSED;
}

bool InputServer::IsButtonJustReleased(int button) {
	return GetButtonState(button) == ActionState::JUST_RELEASED;
}

//-------------------------------------------------------//
//------------------ Input Server -----------------------//
//-------------------------------------------------------//

void InputServer::initialize() {
	GLFWwindow *window = RenderingServer::GetInstance().m_pWindowHandle;

	glfwSetCharCallback(window, CallbackWrapperCharCallback);
	glfwSetKeyCallback(window, CallbackWrapperKeyCallback);
	glfwSetMouseButtonCallback(window, CallbackWrapperMouseButtonCallback);
	glfwSetCursorPosCallback(window, CallbackWrapperCursorPosCallback);
}

void InputServer::mouse_button_callback(GLFWwindow *window, int button, int action, int mods) {
	static std::unordered_map<int, MouseButton> buttons = {
		{GLFW_MOUSE_BUTTON_LEFT, MB_LEFT},
		{GLFW_MOUSE_BUTTON_RIGHT, MB_RIGHT},
		{GLFW_MOUSE_BUTTON_MIDDLE, MB_MIDDLE},
		{GLFW_MOUSE_BUTTON_4, MB_4},
		{GLFW_MOUSE_BUTTON_5, MB_5},
		{GLFW_MOUSE_BUTTON_6, MB_6},
		{GLFW_MOUSE_BUTTON_7, MB_7},
		{GLFW_MOUSE_BUTTON_8, MB_8}};

	if (action == GLFW_PRESS)
		m_buttonStates[buttons[button]] = ActionState::JUST_PRESSED;
	if (action == GLFW_RELEASE)
		m_buttonStates[buttons[button]] = ActionState::JUST_RELEASED;

	InputEventMouseButton event;
	event.button = buttons[button];

	if (action == GLFW_PRESS)
		event.action = PRESSED;
	else if (action == GLFW_RELEASE)
		event.action = RELEASED;

	if (mods & GLFW_MOD_SHIFT)
		event.modifiers.shift = true;
	if (mods & GLFW_MOD_CONTROL)
		event.modifiers.control = true;
	if (mods & GLFW_MOD_ALT)
		event.modifiers.alt = true;
	if (mods & GLFW_MOD_SUPER)
		event.modifiers.super = true;

	App::GetInstance().MouseInputCallback()(event);
}

void InputServer::cursor_pos_callback(GLFWwindow *window, double x, double y) {
	InputEventMouseMove event;

	double deltaX = x - m_input_mouseX;
	double deltaY = y - m_input_mouseY;
	m_input_mouseX = x;
	m_input_mouseY = y;

	event.deltaX = deltaX;
	event.deltaY = deltaY;
	event.mouseX = x;
	event.mouseY = y;

	App::GetInstance().MouseMoveCallback()(event);
}

void InputServer::key_callback(GLFWwindow *window, int key, int scancode, int action, int mods) {
	if (action == GLFW_PRESS)
		m_keyStates[key] = ActionState::JUST_PRESSED;
	else if (action == GLFW_RELEASE)
		m_keyStates[key] = ActionState::JUST_RELEASED;

	InputEventKey event;
	event.key = key;
	event.action = action == GLFW_PRESS	   ? KEY_PRESSED
				   : action == GLFW_REPEAT ? KEY_REPEAT
										   : KEY_RELEASED;

	if (mods & GLFW_MOD_SHIFT)
		event.modifiers.shift = true;
	if (mods & GLFW_MOD_CONTROL)
		event.modifiers.control = true;
	if (mods & GLFW_MOD_ALT)
		event.modifiers.alt = true;
	if (mods & GLFW_MOD_SUPER)
		event.modifiers.super = true;

	App::GetInstance().KeyCallback()(event);
}

void InputServer::char_callback(GLFWwindow *window, unsigned int codepoint) {
	InputEventChar event;
	event.codepoint = codepoint;

	App::GetInstance().CharCallback()(event);
}
