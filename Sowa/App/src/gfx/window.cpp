#include "window.hpp"

#include <GLFW/glfw3.h>
#include "glad/glad.h"

#include "core/application.hpp"
#include "debug.hpp"
#include "utils/event/input_event.hpp"

namespace sowa {
namespace gfx {

Window::Window(EngineContext *ctx) {
	m_ctx = ctx;
}

Window::~Window() {
}

void Window::ErrorCallback(int error, const char *description) {
	Debug::Error("Window error: {}:'{}'", error, description);
}

bool Window::InitWindow(InitWindowArgs args) {
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwSetErrorCallback(ErrorCallback);

	if (args.flags & WindowFlags_::Hidden) {
		glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
	}

	p_window = glfwCreateWindow(args.windowWidth, args.windowHeight, args.title.c_str(), NULL, NULL);
	if (p_window == nullptr) {
		return false;
	}

	glfwMakeContextCurrent(p_window);
	glfwSetWindowUserPointer(p_window, this);
	int version = gladLoadGL();
	if(version == 0) {
		std::cout << "Failed to initailize opengl context" << std::endl;
		return false;
	}
	
	glfwSwapInterval(1);

	if (args.flags & WindowFlags_::Fullscreen) {
		SetFullscreen(true);
	}

	int w, h;
	glfwGetWindowSize(p_window, &w, &h);
	m_windowsize = {w, h};
	m_videosize = {args.videoWidth, args.videoHeight};

	glfwSetKeyCallback(p_window, &KeyCallback);
	glfwSetScrollCallback(p_window, &ScrollCallback);
	glfwSetMouseButtonCallback(p_window, &MouseButtonCallback);
	glfwSetCursorPosCallback(p_window, &CursorPosCallback);
	glfwSetCharCallback(p_window, &CharCallback);
	glfwSetFramebufferSizeCallback(p_window, &FramebufferSizeCallback);

	static gfx::GLTexture cursorTex;
	auto cursorData = File::GetFileContent("res://cursor.png");
	GLTexture::SetFlip(true);
	cursorTex.Load2D(cursorData.data(), cursorData.size());
	GLTexture::SetFlip(false);

	static GLFWimage cursorImage;
	cursorImage.width = cursorTex.Width();
	cursorImage.height = cursorTex.Height();
	cursorImage.pixels = cursorTex.Pixels();


	GLFWcursor* cursor = glfwCreateCursor(&cursorImage, 0, 0);
	glfwSetCursor(p_window, cursor);

	return true;
}

void Window::DestroyWindow() {
	glfwDestroyWindow(p_window);
}

void Window::SetFullscreen(bool fullscreen) {
	if (fullscreen) {
		GLFWmonitor *monitor = glfwGetPrimaryMonitor();
		glfwGetWindowPos(p_window, &m_fullscreen_old_x, &m_fullscreen_old_y);
		glfwGetWindowSize(p_window, &m_fullscreen_old_w, &m_fullscreen_old_h);
		const GLFWvidmode *mode = glfwGetVideoMode(monitor);

		m_fullscreen_old_refresh = mode->refreshRate;
		glfwSetWindowMonitor(p_window, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
		m_fullscreen = true;
	} else {
		glfwSetWindowMonitor(p_window, nullptr, m_fullscreen_old_x, m_fullscreen_old_y, m_fullscreen_old_w, m_fullscreen_old_h, m_fullscreen_old_refresh);
		m_fullscreen = false;
	}
}

void Window::PollEvents() {
	for (auto &[key, state] : m_keyStates) {
		if (state == KeyState::PRESSED)
			m_keyStates[key] = KeyState::DOWN;
		else if (state == KeyState::RELEASED)
			m_keyStates[key] = KeyState::UP;
	}
	for (auto &[button, state] : m_buttonStates) {
		if (state == ButtonState::PRESSED)
			m_buttonStates[button] = ButtonState::DOWN;
		else if (state == ButtonState::RELEASED)
			m_buttonStates[button] = ButtonState::UP;
	}

	m_scrollDelta = {0.f, 0.f};

	glfwPollEvents();
}
void Window::SwapBuffers() {
	glfwSwapBuffers(p_window);
}

bool Window::ShouldClose() {
	return glfwWindowShouldClose(p_window) || IsKeyDown(GLFW_KEY_ESCAPE); // glfwGetKey(p_window, GLFW_KEY_ESCAPE) == GLFW_PRESS;
}

sowa::vec2 Window::GetMousePosition() {
	double x, y;
	glfwGetCursorPos(p_window, &x, &y);
	return sowa::vec2(x, y);
}
sowa::vec2 Window::GetVideoMousePosition() {
	sowa::vec2 pos = GetMousePosition();

	sowa::vec2 ratio = sowa::vec2{
		m_videosize.x / m_windowsize.x,
		m_videosize.y / m_windowsize.y,
	};

	pos.x *= ratio.x;
	pos.y *= ratio.y;

	return pos;
}

bool Window::IsKeyJustPressed(int key) {
	return m_keyStates[key] == KeyState::PRESSED;
}
bool Window::IsKeyJustReleased(int key) {
	return m_keyStates[key] == KeyState::RELEASED;
}
bool Window::IsKeyDown(int key) {
	return m_keyStates[key] == KeyState::PRESSED || m_keyStates[key] == KeyState::DOWN;
}

bool Window::IsButtonJustPressed(int button) {
	return m_buttonStates[button] == ButtonState::PRESSED;
}
bool Window::IsButtonJustReleased(int button) {
	return m_buttonStates[button] == ButtonState::RELEASED;
}
bool Window::IsButtonDown(int button) {
	return m_buttonStates[button] == ButtonState::PRESSED || m_buttonStates[button] == ButtonState::DOWN;
}

double Window::GetScrollDeltaY() {
	return m_scrollDelta.y;
}
double Window::GetScrollDeltaX() {
	return m_scrollDelta.x;
}

bool Window::SetWindowIcon(GLTexture& icon) {
	if (icon.Channels() != 4) {
		Debug::Error("Window::SetWindowIcon : invalid format, texture must have 4 channels. (given texture has {})", icon.Channels());
		return false;
	}

	GLFWimage image[1];
	image[0].width = icon.Width();
	image[0].height = icon.Height();
	image[0].pixels = icon.Pixels();

	if (image[0].pixels != nullptr) {
		glfwSetWindowIcon(p_window, 1, image);
		return true;
	} else {
		Debug::Error("Invalid icon on Window::SetWindowIcon");
		return false;
	}
}

void Window::FramebufferSizeCallback(GLFWwindow *pwindow, int width, int height) {
	Window *window = reinterpret_cast<Window *>(glfwGetWindowUserPointer(pwindow));
	window->m_windowsize = {width, height};
}

void Window::KeyCallback(GLFWwindow *pwindow, int key, int scancode, int action, int mods) {
	Window *window = reinterpret_cast<Window *>(glfwGetWindowUserPointer(pwindow));

	if (action == GLFW_PRESS) {
		window->m_keyStates[key] = KeyState::PRESSED;
	} else if (action == GLFW_RELEASE) {
		window->m_keyStates[key] = KeyState::RELEASED;
	}

	sowa::InputEvent e;
	e._Type = InputEventType::Key;
	e.key.key = key;
	e.key.scanCode = scancode;
	e.key.action = action;
	e.key.modifiers = mods;

	window->m_ctx->GetSingleton<sowa::Application>(sowa::Server::APPLICATION)->OnInput().Invoke(e);
}
void Window::ScrollCallback(GLFWwindow *pwindow, double xOffset, double yOffset) {
	Window *window = reinterpret_cast<Window *>(glfwGetWindowUserPointer(pwindow));

	window->m_scrollDelta.x += xOffset;
	window->m_scrollDelta.y += yOffset;

	InputEvent e;
	e._Type = InputEventType::Scroll;
	e.scroll.scrollX = xOffset;
	e.scroll.scrollY = yOffset;

	window->m_ctx->GetSingleton<sowa::Application>(sowa::Server::APPLICATION)->OnInput().Invoke(e);
}
void Window::MouseButtonCallback(GLFWwindow *pwindow, int button, int action, int mods) {
	Window *window = reinterpret_cast<Window *>(glfwGetWindowUserPointer(pwindow));

	if (action == GLFW_PRESS) {
		window->m_buttonStates[button] = ButtonState::PRESSED;
	} else if (action == GLFW_RELEASE) {
		window->m_buttonStates[button] = ButtonState::RELEASED;
	}

	InputEvent e;
	e._Type = InputEventType::MouseButton;

	e.mouseButton.button = button;
	e.mouseButton.action = action;
	e.mouseButton.modifiers = mods;

	window->m_ctx->GetSingleton<sowa::Application>(sowa::Server::APPLICATION)->OnInput().Invoke(e);

	// Click detection
	static std::chrono::milliseconds::rep last_press = 0;
	static std::chrono::milliseconds::rep last_release = 0;

	const int single_click_interval = 230; // ms
	const int double_click_interval = 500; // ms

	if (action == GLFW_PRESS) {
		std::chrono::milliseconds::rep milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();

		if (milliseconds < last_press + double_click_interval) {
			// Double click
			InputEvent e;
			e._Type = InputEventType::MouseClick;

			e.mouseClick.button = button;
			e.mouseClick.single = false;
			e.mouseClick.modifiers = mods;
			window->m_ctx->GetSingleton<sowa::Application>(sowa::Server::APPLICATION)->OnInput().Invoke(e);
		}

		last_press = milliseconds;
	}

	if (action == GLFW_RELEASE) {
		std::chrono::milliseconds::rep milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();

		if (milliseconds < last_press + single_click_interval) {
			// Single click
			InputEvent e;
			e._Type = InputEventType::MouseClick;

			e.mouseClick.button = button;
			e.mouseClick.single = true;
			e.mouseClick.modifiers = mods;
			window->m_ctx->GetSingleton<sowa::Application>(sowa::Server::APPLICATION)->OnInput().Invoke(e);
		}

		last_release = milliseconds;
	}
}
void Window::CursorPosCallback(GLFWwindow *pwindow, double xPos, double yPos) {
	Window *window = reinterpret_cast<Window *>(glfwGetWindowUserPointer(pwindow));

	sowa::vec2 delta;
	delta.x = window->m_mousePos.x - xPos;
	delta.y = window->m_mousePos.y - yPos;
	window->m_mousePos = {xPos, yPos};

	InputEvent e;
	e._Type = InputEventType::MouseMove;

	e.mouseMove.deltaX = delta.x;
	e.mouseMove.deltaY = delta.y;
	e.mouseMove.posX = xPos;
	e.mouseMove.posY = yPos;

	window->m_ctx->GetSingleton<sowa::Application>(sowa::Server::APPLICATION)->OnInput().Invoke(e);
}
void Window::CharCallback(GLFWwindow *pwindow, unsigned int codePoint) {
	Window *window = reinterpret_cast<Window *>(glfwGetWindowUserPointer(pwindow));

	InputEvent e;
	e._Type = InputEventType::Character;

	e.character.codePoint = codePoint;

	window->m_ctx->GetSingleton<sowa::Application>(sowa::Server::APPLICATION)->OnInput().Invoke(e);
}

} // namespace gfx
} // namespace sowa
