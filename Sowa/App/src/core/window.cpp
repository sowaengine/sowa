#include "core/window.hpp"
#include "sowa.hpp"

#include "GLFW/glfw3.h"
#include "stlpch.hpp"

#include "resource/texture/texture.hpp"

#include "core/application.hpp"
#include "core/engine_context.hpp"
#include "core/input.hpp"

#include "Core/nm_Window.hpp"

namespace Sowa {
Window::Window() {}
Window::~Window() {}

struct WindowCallback {
	static void KeyCallback(GLFWwindow *window, int key, int scancode, int action, int mods);
	static void ScrollCallback(GLFWwindow *window, double xOffset, double yOffset);
};

class WindowAccessor {
  public:
	WindowAccessor(GLFWwindow *window);
	~WindowAccessor() {}

	WindowAccessor &RegisterKeyEvent(int key, int scancode, int action, int mods);
	WindowAccessor &RegisterScrollEvent(double xOffset, double yOffset);

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
}

void Window::UpdateEvents() {
	for (auto &[key, state] : _KeyStates) {
		if (state == KeyState::PRESSED)
			_KeyStates[key] = KeyState::DOWN;
		else if (state == KeyState::RELEASED)
			_KeyStates[key] = KeyState::UP;
	}

	_ScrollDeltaX = 0.0;
	_ScrollDeltaY = 0.0;
}

/* -- Input -- */
Vec2 Window::GetMousePosition() {
	double x, y;
	glfwGetCursorPos(_windowHandle->GetGLFWwindow(), &x, &y);

	// x -= GetWindowWidth() * 0.5f;
	// y -= GetWindowHeight() * 0.5f;

#ifndef SW_EDITOR
	x -= GetBlackbarWidth();
	y -= GetBlackbarHeight();
#else
	Vec2 scale;
	scale.x = (float)GetWindowWidth() / (float)Editor_GetWindowWidth();
	scale.y = (float)GetWindowHeight() / (float)Editor_GetWindowHeight();

	x -= Editor_GetWindowPosX();
	y -= Editor_GetWindowPosY();
	x *= scale.x;
	y *= scale.y;
#endif

	return Vec2(x, y);
}

Vec2 Window::GetGameMousePosition() {

	Vec2 scale;
#ifndef SW_EDITOR
	scale.x = (float)GetVideoWidth() / ((float)GetWindowWidth() - (GetBlackbarWidth() * 2));
	scale.y = (float)GetVideoHeight() / ((float)GetWindowHeight() - (GetBlackbarHeight() * 2));
#else

	scale.x = (float)GetVideoWidth() / (float)GetWindowWidth();
	scale.y = (float)GetVideoHeight() / (float)GetWindowHeight();
#endif

	Vec2 mousePos = GetMousePosition();
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
	Sowa::Window &window = _Ctx->GetSingleton<Application>(Sowa::Server::APPLICATION)->GetWindow();

	if (action == GLFW_PRESS)
		window._KeyStates[key] = KeyState::PRESSED;
	else if (action == GLFW_RELEASE)
		window._KeyStates[key] = KeyState::RELEASED;

	return *this;
}
WindowAccessor &WindowAccessor::RegisterScrollEvent(double xOffset, double yOffset) {
	Sowa::Window &window = _Ctx->GetSingleton<Application>(Sowa::Server::APPLICATION)->GetWindow();

	window._ScrollDeltaY = yOffset;
	window._ScrollDeltaX += xOffset;

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

} // namespace Sowa