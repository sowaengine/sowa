#include "core/window.hpp"
#include "sowa.hpp"

#include <chrono>

#include "GLFW/glfw3.h"
#include "stlpch.hpp"

#include "resource/texture/image_texture.hpp"

#include "core/application.hpp"
#include "core/engine_context.hpp"
#include "core/input.hpp"

#include "Core/nm_Window.hpp"

namespace sowa {
Window::Window() {}
Window::~Window() {}

struct WindowCallback {
	static void KeyCallback(GLFWwindow *window, int key, int scancode, int action, int mods);
	static void ScrollCallback(GLFWwindow *window, double xOffset, double yOffset);
	static void MouseButtonCallback(GLFWwindow *window, int button, int action, int mods);
	static void CursorPosCallback(GLFWwindow *window, double xPos, double yPos);
	static void CharCallback(GLFWwindow *window, unsigned int codePoint);
};

class WindowAccessor {
  public:
	WindowAccessor(GLFWwindow *window);
	~WindowAccessor() {}

	WindowAccessor &RegisterKeyEvent(int key, int scancode, int action, int mods);
	WindowAccessor &RegisterScrollEvent(double xOffset, double yOffset);
	WindowAccessor &RegisterButtonEvent(int button, int action, int mods);
	WindowAccessor &RegisterClickEvent(int button, bool single, int mods);
	WindowAccessor &RegisterMousePosEvent(double xPos, double yPos);
	WindowAccessor &RegisterCharEvent(unsigned int codePoint);

  private:
	void LoadContext();

	EngineContext *_Ctx{nullptr};
	GLFWwindow *_Window{nullptr};
};

void Window::InitWindow(nmGfx::Window &window, EngineContext &ctx) {
	_windowHandle = &window;
	_Ctx = &ctx;

	glfwSetWindowUserPointer(_windowHandle->GetGLFWwindow(), reinterpret_cast<void *>(&ctx));
	glfwSetKeyCallback(_windowHandle->GetGLFWwindow(), &WindowCallback::KeyCallback);
	glfwSetScrollCallback(_windowHandle->GetGLFWwindow(), &WindowCallback::ScrollCallback);
	glfwSetMouseButtonCallback(_windowHandle->GetGLFWwindow(), &WindowCallback::MouseButtonCallback);
	glfwSetCursorPosCallback(_windowHandle->GetGLFWwindow(), &WindowCallback::CursorPosCallback);
	glfwSetCharCallback(_windowHandle->GetGLFWwindow(), &WindowCallback::CharCallback);
}

void Window::UpdateEvents() {
	for (auto &[key, state] : _KeyStates) {
		if (state == KeyState::PRESSED)
			_KeyStates[key] = KeyState::DOWN;
		else if (state == KeyState::RELEASED)
			_KeyStates[key] = KeyState::UP;
	}
	for (auto &[button, state] : _ButtonStates) {
		if (state == ButtonState::PRESSED)
			_ButtonStates[button] = ButtonState::DOWN;
		else if (state == ButtonState::RELEASED)
			_ButtonStates[button] = ButtonState::UP;
	}

	_ScrollDeltaX = 0.0;
	_ScrollDeltaY = 0.0;
}

/* -- Input -- */
vec2 Window::GetMousePosition() {
	double x, y;
	glfwGetCursorPos(_windowHandle->GetGLFWwindow(), &x, &y);

	// x -= GetWindowWidth() * 0.5f;
	// y -= GetWindowHeight() * 0.5f;

#ifndef SW_EDITOR
	x -= GetBlackbarWidth();
	y -= GetBlackbarHeight();
#else
	vec2 scale;
	scale.x = (float)GetWindowWidth() / (float)Editor_GetWindowWidth();
	scale.y = (float)GetWindowHeight() / (float)Editor_GetWindowHeight();

	x -= Editor_GetWindowPosX();
	y -= Editor_GetWindowPosY();
	x *= scale.x;
	y *= scale.y;
#endif

	return vec2(x, y);
}

vec2 Window::GetGameMousePosition() {

	vec2 scale;
#ifndef SW_EDITOR
	scale.x = (float)GetVideoWidth() / ((float)GetWindowWidth() - (GetBlackbarWidth() * 2));
	scale.y = (float)GetVideoHeight() / ((float)GetWindowHeight() - (GetBlackbarHeight() * 2));
#else

	scale.x = (float)GetVideoWidth() / (float)GetWindowWidth();
	scale.y = (float)GetVideoHeight() / (float)GetWindowHeight();
#endif

	vec2 mousePos = GetMousePosition();
	mousePos.x *= scale.x;
	mousePos.y *= scale.y;

	return mousePos;
}

bool Window::IsKeyJustPressed(int key) {
	return _KeyStates[key] == KeyState::PRESSED;
}
bool Window::IsKeyJustReleased(int key) {
	return _KeyStates[key] == KeyState::RELEASED;
}

bool Window::IsKeyDown(int key) {
	return _KeyStates[key] == KeyState::DOWN;
}
bool Window::IsKeyUp(int key) {
	return _KeyStates[key] == KeyState::UP;
}

bool Window::IsButtonJustPressed(int button) {
	return _ButtonStates[button] == ButtonState::PRESSED;
}
bool Window::IsButtonJustReleased(int button) {
	return _ButtonStates[button] == ButtonState::RELEASED;
}

bool Window::IsButtonDown(int button) {
	return _ButtonStates[button] == ButtonState::DOWN;
}
bool Window::IsButtonUp(int button) {
	return _ButtonStates[button] == ButtonState::UP;
}

double Window::GetScrollDeltaY() {
	return _ScrollDeltaY;
}
double Window::GetScrollDeltaX() {
	return _ScrollDeltaX;
}

//
//
int Window::GetVideoWidth() { return _windowHandle->GetVideoWidth(); }
int Window::GetVideoHeight() { return _windowHandle->GetVideoHeight(); }

int Window::GetWindowWidth() { return _windowHandle->GetWindowWidth(); }
int Window::GetWindowHeight() { return _windowHandle->GetWindowHeight(); }

bool Window::ShouldClose() {
	return _windowHandle->ShouldClose();
}

bool Window::SetWindowIcon(Reference<ImageTexture> icon) {
	if (icon == nullptr) {
		Debug::Error("Invalid icon on Window::SetWindowIcon");
		return false;
	}

	if (icon->Channels() != 4) {
		Debug::Error("Window::SetWindowIcon : invalid format, texture must have 4 channels. (given texture has {})", icon->Channels());
		return false;
	}

	GLFWimage image[1];
	image[0].width = icon->Width();
	image[0].height = icon->Height();
	image[0].pixels = icon->Pixels();

	if (image[0].pixels != nullptr) {
		glfwSetWindowIcon(_windowHandle->GetGLFWwindow(), 1, image);
		return true;
	} else {
		Debug::Error("Invalid icon on Window::SetWindowIcon");
		return false;
	}
}

#ifndef SW_EDITOR
int Window::GetBlackbarWidth() {
	float scale = MIN((float)GetWindowWidth() / GetVideoWidth(), (float)GetWindowHeight() / GetVideoHeight());
	return (GetWindowWidth() - ((float)GetVideoWidth() * scale)) * 0.5f;
}
int Window::GetBlackbarHeight() {
	float scale = MIN((float)GetWindowWidth() / GetVideoWidth(), (float)GetWindowHeight() / GetVideoHeight());
	return (GetWindowHeight() - ((float)GetVideoHeight() * scale)) * 0.5f;
}
#else
int Window::GetBlackbarWidth() {
	return m_Editor_BlackbarWidth;
}
int Window::GetBlackbarHeight() {
	return m_Editor_BlackbarHeight;
}
#endif

//
//
WindowAccessor::WindowAccessor(GLFWwindow *window) {
	_Window = window;
	LoadContext();
}
void WindowAccessor::LoadContext() {
	_Ctx = reinterpret_cast<EngineContext *>(glfwGetWindowUserPointer(_Window));
}

WindowAccessor &WindowAccessor::RegisterKeyEvent(int key, int scancode, int action, int mods) {
	auto *app = _Ctx->GetSingleton<Application>(sowa::Server::APPLICATION);
	sowa::Window &window = app->GetWindow();

	if (action == GLFW_PRESS)
		window._KeyStates[key] = KeyState::PRESSED;
	else if (action == GLFW_RELEASE)
		window._KeyStates[key] = KeyState::RELEASED;

	InputEvent e;
	e._Type = InputEventType::Key;
	e.key.key = key;
	e.key.scanCode = scancode;
	e.key.action = action;
	e.key.modifiers = mods;

	app->OnInput().Invoke(e);
	return *this;
}
WindowAccessor &WindowAccessor::RegisterScrollEvent(double xOffset, double yOffset) {
	auto *app = _Ctx->GetSingleton<Application>(sowa::Server::APPLICATION);
	sowa::Window &window = app->GetWindow();

	window._ScrollDeltaY += yOffset;
	window._ScrollDeltaX += xOffset;

	InputEvent e;
	e._Type = InputEventType::Scroll;
	e.scroll.scrollX = xOffset;
	e.scroll.scrollY = yOffset;

	app->OnInput().Invoke(e);
	return *this;
}

WindowAccessor &WindowAccessor::RegisterButtonEvent(int button, int action, int mods) {
	auto *app = _Ctx->GetSingleton<Application>(sowa::Server::APPLICATION);
	sowa::Window &window = app->GetWindow();

	if (action == GLFW_PRESS)
		window._ButtonStates[button] = ButtonState::PRESSED;
	else if (action == GLFW_RELEASE)
		window._ButtonStates[button] = ButtonState::RELEASED;

	InputEvent e;
	e._Type = InputEventType::MouseButton;

	e.mouseButton.button = button;
	e.mouseButton.action = action;
	e.mouseButton.modifiers = mods;

	app->OnInput().Invoke(e);
	return *this;
}

WindowAccessor &WindowAccessor::RegisterClickEvent(int button, bool single, int mods) {
	auto *app = _Ctx->GetSingleton<Application>(sowa::Server::APPLICATION);
	sowa::Window &window = app->GetWindow();

	InputEvent e;
	e._Type = InputEventType::MouseClick;

	e.mouseClick.button = button;
	e.mouseClick.single = single;
	e.mouseClick.modifiers = mods;

	app->OnInput().Invoke(e);
	return *this;
}

WindowAccessor &WindowAccessor::RegisterMousePosEvent(double xPos, double yPos) {
	auto *app = _Ctx->GetSingleton<Application>(sowa::Server::APPLICATION);
	sowa::Window &window = app->GetWindow();

	double deltaX = window._MousePosX - xPos;
	double deltaY = window._MousePosY - yPos;
	window._MousePosX = xPos;
	window._MousePosY = yPos;

	InputEvent e;
	e._Type = InputEventType::MouseMove;

	e.mouseMove.deltaX = deltaX;
	e.mouseMove.deltaY = deltaY;
	e.mouseMove.posX = xPos;
	e.mouseMove.posY = yPos;

	app->OnInput().Invoke(e);
	return *this;
}

WindowAccessor &WindowAccessor::RegisterCharEvent(unsigned int codePoint) {
	auto *app = _Ctx->GetSingleton<Application>(sowa::Server::APPLICATION);
	sowa::Window &window = app->GetWindow();

	InputEvent e;
	e._Type = InputEventType::Character;

	e.character.codePoint = codePoint;

	app->OnInput().Invoke(e);
	return *this;
}

//
//

void WindowCallback::KeyCallback(GLFWwindow *window, int key, int scancode, int action, int mods) {
	WindowAccessor(window).RegisterKeyEvent(key, scancode, action, mods);
}
void WindowCallback::ScrollCallback(GLFWwindow *window, double xOffset, double yOffset) {
	WindowAccessor(window).RegisterScrollEvent(xOffset, yOffset);
}
void WindowCallback::MouseButtonCallback(GLFWwindow *window, int button, int action, int mods) {
	WindowAccessor(window).RegisterButtonEvent(button, action, mods);

	static std::chrono::milliseconds::rep last_press = 0;
	static std::chrono::milliseconds::rep last_release = 0;

	const int single_click_interval = 230; // ms
	const int double_click_interval = 500; // ms

	if (action == GLFW_PRESS) {
		std::chrono::milliseconds::rep milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();

		if (milliseconds < last_press + double_click_interval) {
			// Double click
			WindowAccessor(window).RegisterClickEvent(button, false, mods);
		}

		last_press = milliseconds;
	}

	if (action == GLFW_RELEASE) {
		std::chrono::milliseconds::rep milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();

		if (milliseconds < last_press + single_click_interval) {
			// Single click
			WindowAccessor(window).RegisterClickEvent(button, true, mods);
		}

		last_release = milliseconds;
	}
}
void WindowCallback::CursorPosCallback(GLFWwindow *window, double xPos, double yPos) {
	WindowAccessor(window).RegisterMousePosEvent(xPos, yPos);
}
void WindowCallback::CharCallback(GLFWwindow *window, unsigned int codePoint) {
	WindowAccessor(window).RegisterCharEvent(codePoint);
}

} // namespace sowa