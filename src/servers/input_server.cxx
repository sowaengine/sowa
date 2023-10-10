#include "input_server.hxx"
#include "core/app.hxx"
#include "core/input.hxx"
#include "gui_server.hxx"
#include "rendering_server.hxx"

static bool s_blockMoveMoveEvent = false;

struct InputCallbackBridge {
	InputCallbackBridge() = default;

	void MouseButtonCallback(GLFWwindow *window, int button, int action, int mods) {
		InputServer::get().mouse_button_callback(window, button, action, mods);
	}

	void CursorPosCalllback(GLFWwindow *window, double x, double y) {
		InputServer::get().cursor_pos_callback(window, x, y);
	}

	void KeyCallback(GLFWwindow *window, int key, int scancode, int action, int mods) {
		InputServer::get().key_callback(window, key, scancode, action, mods);
	}

	void CharCallback(GLFWwindow *window, unsigned int codepoint) {
		InputServer::get().char_callback(window, codepoint);
	}

	void ScrollCallback(GLFWwindow *window, double xOffset, double yOffset) {
		InputServer::get().scroll_callback(window, xOffset, yOffset);
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

static void CallbackWrapperSrollCallback(GLFWwindow *window, double xOffset, double yOffset) {
	InputCallbackBridge().ScrollCallback(window, xOffset, yOffset);
}

InputServer::InputServer() {
}

InputServer &InputServer::get() {
	static InputServer *server = new InputServer;
	return *server;
}

void InputServer::ProcessInput() {
}

void InputServer::PollEvents() {
	//
	m_pressedChar = 0;
	m_buttonSingleClicked.clear();
	m_buttonDoubleClicked.clear();
	m_mouseMotionX = 0.0;
	m_mouseMotionY = 0.0;

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

	if (RenderingServer::get().m_currentCursorMode == CursorMode::Tiled) {
		vec2 windowSize = RenderingServer::get().GetWindowSize();

		bool setCursor = false;
		vec2 mousePos = Input::GetWindowMousePosition();

		const float padding = 2.f;
		if (mousePos.x < padding) {
			mousePos.x = windowSize.x - padding;
			setCursor = true;
		} else if (mousePos.x > windowSize.x - padding) {
			mousePos.x = padding;
			setCursor = true;
		}

		if (mousePos.y < padding) {
			mousePos.y = windowSize.y - padding;
			setCursor = true;
		} else if (mousePos.y > windowSize.y - padding) {
			mousePos.y = padding;
			setCursor = true;
		}

		if (setCursor) {
			s_blockMoveMoveEvent = true;
			glfwSetCursorPos(RenderingServer::get().m_pWindowHandle, mousePos.x, mousePos.y);
			m_input_mouseX = mousePos.x;
			m_input_mouseY = mousePos.y;
		}
	}

	//
	glfwPollEvents();
	s_blockMoveMoveEvent = false;
}

vec2 InputServer::GetMousePosition() {
	return vec2(
		m_input_mouseX,
		RenderingServer::get().GetWindowSize().y - m_input_mouseY);
}

vec2 InputServer::GetWindowMousePosition() {
	return vec2(
		m_input_mouseX,
		m_input_mouseY);
}

vec2 InputServer::GetGameMousePosition() {
	vec2 windowSize = RenderingServer::get().GetWindowSize();

	return vec2(
		m_input_mouseX * (1920.f / float(windowSize.x)),
		1080.f - m_input_mouseY * (1080.f / float(windowSize.y)));
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
	return state == ActionState::JUST_RELEASED || state == ActionState::UP || state == ActionState::NONE;
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
	return state == ActionState::JUST_PRESSED || state == ActionState::DOWN;
}

bool InputServer::IsButtonUp(int button) {
	ActionState state = GetButtonState(button);
	return state == ActionState::JUST_RELEASED || state == ActionState::UP || state == ActionState::NONE;
}

bool InputServer::IsButtonJustPressed(int button) {
	return IsCursorInside() && GetButtonState(button) == ActionState::JUST_PRESSED;
}

bool InputServer::IsButtonJustReleased(int button) {
	return GetButtonState(button) == ActionState::JUST_RELEASED;
}

bool InputServer::IsButtonJustClicked(int button) {
	return m_buttonSingleClicked[button];
}

bool InputServer::IsButtonJustDoubleClicked(int button) {
	return m_buttonDoubleClicked[button];
}

vec2 InputServer::GetMouseMotion() {
	return vec2(m_mouseMotionX, m_mouseMotionY);
}

//-------------------------------------------------------//
//------------------ Input Server -----------------------//
//-------------------------------------------------------//

void InputServer::initialize() {
	GLFWwindow *window = RenderingServer::get().m_pWindowHandle;

	glfwSetCharCallback(window, CallbackWrapperCharCallback);
	glfwSetKeyCallback(window, CallbackWrapperKeyCallback);
	glfwSetMouseButtonCallback(window, CallbackWrapperMouseButtonCallback);
	glfwSetCursorPosCallback(window, CallbackWrapperCursorPosCallback);
	glfwSetScrollCallback(window, CallbackWrapperSrollCallback);
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
	MouseButton btn = event.button;

	static std::unordered_map<MouseButton, std::chrono::high_resolution_clock::time_point> pressTimes;
	const std::chrono::milliseconds doubleClickDelay = std::chrono::milliseconds(200);
	const std::chrono::milliseconds clickDelay = std::chrono::milliseconds(100);

	auto now = std::chrono::high_resolution_clock::now();

	if (action == GLFW_PRESS) {
		std::chrono::milliseconds delay = std::chrono::duration_cast<std::chrono::milliseconds>(now - pressTimes[event.button]);

		if (delay < doubleClickDelay) {
			InputEventClick event;
			event.button = btn;
			event.doubleClick = true;

			m_buttonDoubleClicked[btn] = true;

			if (GuiServer::get().on_viewport)
				App::get().ClickCallback()(event);
		}
	}
	if (action == GLFW_RELEASE) {
		std::chrono::milliseconds delay = std::chrono::duration_cast<std::chrono::milliseconds>(now - pressTimes[event.button]);

		if (delay < clickDelay) {
			InputEventClick event;
			event.button = btn;
			event.doubleClick = false;

			m_buttonSingleClicked[btn] = true;

			if (GuiServer::get().on_viewport)
				App::get().ClickCallback()(event);
		}
	}

	if (action == GLFW_PRESS) {
		pressTimes[event.button] = now;
	}

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

	if (GuiServer::get().on_viewport)
		App::get().MouseInputCallback()(event);
}

void InputServer::cursor_pos_callback(GLFWwindow *window, double x, double y) {
	if (s_blockMoveMoveEvent) {
		return;
	}

	InputEventMouseMove event;

	vec2 windowSize = RenderingServer::get().GetWindowSize();
	x -= GuiServer::get().m_viewport_rect.x;
	y -= GuiServer::get().m_viewport_rect.y;

	rect viewport = GuiServer::get().m_viewport_rect;
	vec2 point = rect(0.f, 0.f, viewport.w, viewport.h).map_point(vec2(x, y), rect(0.f, 0.f, windowSize.x, windowSize.y));
	x = point.x;
	y = point.y;

	double deltaX = x - m_input_mouseX;
	double deltaY = y - m_input_mouseY;
	m_input_mouseX = x;
	m_input_mouseY = y;

	event.deltaX = deltaX;
	event.deltaY = deltaY;
	event.mouseX = x;
	event.mouseY = y;

	m_mouseMotionX = deltaX;
	m_mouseMotionY = deltaY;

	App::get().MouseMoveCallback()(event);
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

	App::get().KeyCallback()(event);
}

void InputServer::char_callback(GLFWwindow *window, unsigned int codepoint) {
	InputEventChar event;
	event.codepoint = codepoint;

	App::get().CharCallback()(event);
}

void InputServer::scroll_callback(GLFWwindow *window, double xOffset, double yOffset) {
	if (!GuiServer::get().on_viewport)
		return;

	InputEventScroll event;
	event.xOffset = xOffset;
	event.yOffset = yOffset;

	App::get().ScrollCallback()(event);
}

bool InputServer::IsCursorInside() {
	return GuiServer::get().on_viewport;
}