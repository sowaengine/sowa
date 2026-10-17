#include "core/window.hpp"

#include <string>
#include <utility>

#include <GLFW/glfw3.h>
#include <stb_image.h>

#include "core/input.hpp"
#include "core/log.hpp"
#include "core/scope_exit.hpp"

namespace sowa {
namespace {

bool g_glfwInitialized = false;
int g_glfwWindowCount = 0;
thread_local bool g_readingClipboard = false;

KeyAction ToKeyAction(int action) {
	switch (action) {
	case GLFW_PRESS:
		return KeyAction::Press;
	case GLFW_RELEASE:
		return KeyAction::Release;
	case GLFW_REPEAT:
		return KeyAction::Repeat;
	default:
		return KeyAction::Release;
	}
}

} // namespace

Window::Window() = default;

Window::~Window() {
	Destroy();
}

bool Window::EnsureGlfwInitialized() {
	if (g_glfwInitialized) {
		return true;
	}

	glfwSetErrorCallback(ErrorCallback);
	if (!glfwInit()) {
		Log::Error("Failed to initialize GLFW");
		return false;
	}
	g_glfwInitialized = true;
	return true;
}

bool Window::Create(i32 width, i32 height, std::string_view title) {
	if (_window) {
		Destroy();
	}

	if (!EnsureGlfwInitialized()) {
		return false;
	}

	const std::string titleStr(title);
	_window = glfwCreateWindow(width, height, titleStr.c_str(), nullptr, nullptr);
	if (!_window) {
		Log::Error("Failed to create window ({}x{}, title='{}')", width, height, titleStr);
		return false;
	}
	++g_glfwWindowCount;

	glfwSetWindowUserPointer(_window, this);
	glfwSetFramebufferSizeCallback(_window, FramebufferSizeCallback);
	glfwSetKeyCallback(_window, KeyCallback);
	glfwSetMouseButtonCallback(_window, MouseButtonCallback);
	glfwSetCursorPosCallback(_window, CursorPosCallback);
	glfwSetScrollCallback(_window, ScrollCallback);
	glfwSetCharCallback(_window, CharCallback);

	glfwGetFramebufferSize(_window, &_framebufferWidth, &_framebufferHeight);
	return true;
}

void Window::Destroy() {
	if (_window) {
		glfwDestroyWindow(_window);
		_window = nullptr;
		--g_glfwWindowCount;
	}

	_input = nullptr;
	_framebufferWidth = 0;
	_framebufferHeight = 0;

	if (g_glfwWindowCount == 0 && g_glfwInitialized) {
		glfwTerminate();
		g_glfwInitialized = false;
	}
}

bool Window::ShouldClose() const {
	if (!_window) {
		return true;
	}
	return glfwWindowShouldClose(_window) != 0;
}

void Window::SetShouldClose(bool close) {
	if (!_window) {
		return;
	}
	glfwSetWindowShouldClose(_window, close ? GLFW_TRUE : GLFW_FALSE);
}

Vector2 Window::GetSize() const {
	return Vector2(static_cast<f32>(_framebufferWidth), static_cast<f32>(_framebufferHeight));
}

Vector2 Window::GetContentSize() const {
	if (!_window) {
		return Vector2(0.f, 0.f);
	}
	int width = 0;
	int height = 0;
	glfwGetWindowSize(_window, &width, &height);
	return Vector2(static_cast<f32>(width), static_cast<f32>(height));
}

void Window::SetSize(i32 width, i32 height) {
	if (!_window) {
		return;
	}
	glfwSetWindowSize(_window, width, height);
}

void Window::SetTitle(std::string_view title) {
	if (!_window) {
		return;
	}
	const std::string titleStr(title);
	glfwSetWindowTitle(_window, titleStr.c_str());
}

bool Window::SetIcon(const std::byte *data, std::size_t size) {
	if (!_window) {
		return false;
	}
	int width = 0;
	int height = 0;
	int channels = 0;
	stbi_set_flip_vertically_on_load(0);
	stbi_uc *pixels = stbi_load_from_memory(reinterpret_cast<const stbi_uc *>(data),
		static_cast<int>(size), &width, &height, &channels, 4);
	if (pixels == nullptr) {
		Log::Warn("Window icon: {}", stbi_failure_reason());
		return false;
	}
	const GLFWimage image{width, height, pixels};
	glfwSetWindowIcon(_window, 1, &image);
	stbi_image_free(pixels);
	return true;
}

void Window::PollEvents() {
	glfwPollEvents();
}

void Window::SwapBuffers() {
	if (!_window) {
		return;
	}
	glfwSwapBuffers(_window);
}

void Window::ErrorCallback(int error, const char *description) {
	// Empty or non-text clipboard contents are normal when checking whether Paste is available.
	if (g_readingClipboard && error == GLFW_FORMAT_UNAVAILABLE) {
		return;
	}
	Log::Error("GLFW error {}: {}", error, description ? description : "");
}

void Window::FramebufferSizeCallback(GLFWwindow *window, int width, int height) {
	auto *self = static_cast<Window *>(glfwGetWindowUserPointer(window));
	if (self) {
		self->onFramebufferResize(width, height);
	}
}

void Window::KeyCallback(GLFWwindow *window, int key, int /*scancode*/, int action, int mods) {
	auto *self = static_cast<Window *>(glfwGetWindowUserPointer(window));
	if (!self || !self->_input) {
		return;
	}
	self->_input->HandleKey(static_cast<Key>(key), ToKeyAction(action), mods);
}

void Window::MouseButtonCallback(GLFWwindow *window, int button, int action, int mods) {
	auto *self = static_cast<Window *>(glfwGetWindowUserPointer(window));
	if (!self || !self->_input) {
		return;
	}
	self->_input->HandleMouseButton(button, ToKeyAction(action), mods);
}

void Window::CursorPosCallback(GLFWwindow *window, double x, double y) {
	auto *self = static_cast<Window *>(glfwGetWindowUserPointer(window));
	if (!self || !self->_input) {
		return;
	}
	self->_input->HandleCursorPos(static_cast<f32>(x), static_cast<f32>(y));
}

void Window::ScrollCallback(GLFWwindow *window, double xOffset, double yOffset) {
	auto *self = static_cast<Window *>(glfwGetWindowUserPointer(window));
	if (!self || !self->_input) {
		return;
	}
	self->_input->HandleScroll(static_cast<f32>(xOffset), static_cast<f32>(yOffset));
}

void Window::CharCallback(GLFWwindow *window, unsigned int codepoint) {
	auto *self = static_cast<Window *>(glfwGetWindowUserPointer(window));
	if (!self || !self->_input) {
		return;
	}
	self->_input->HandleChar(codepoint);
}

void Window::onFramebufferResize(i32 width, i32 height) {
	_framebufferWidth = width;
	_framebufferHeight = height;
}

std::string Window::GetClipboard() const {
	if (!_window) {
		return {};
	}
	const bool wasReadingClipboard = std::exchange(g_readingClipboard, true);
	const auto restore =
		MakeScopeExit([wasReadingClipboard] { g_readingClipboard = wasReadingClipboard; });
	const char *text = glfwGetClipboardString(_window);
	return text ? text : "";
}

void Window::SetClipboard(const std::string &text) {
	if (_window) {
		glfwSetClipboardString(_window, text.c_str());
	}
}

} // namespace sowa
