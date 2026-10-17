#pragma once

#include <cstddef>
#include <string>
#include <string_view>

#include "core/types.hpp"
#include "math/vector2.hpp"

struct GLFWwindow;

namespace sowa {

class Input;

// Owns a GLFW window. SetInput connects its input callbacks.
class Window {
  public:
	Window();
	~Window();

	Window(const Window &) = delete;
	Window &operator=(const Window &) = delete;
	Window(Window &&) = delete;
	Window &operator=(Window &&) = delete;

	// Initializes GLFW once (safe to call repeatedly). Required before
	// GraphicsDevice::ApplyWindowHints() and Window::Create().
	static bool EnsureGlfwInitialized();

	// Creates or replaces the window. Set graphics context hints first. Returns false on failure.
	bool Create(i32 width, i32 height, std::string_view title);
	void Destroy();

	bool IsCreated() const {
		return _window != nullptr;
	}

	bool ShouldClose() const;
	void SetShouldClose(bool close = true);

	// Framebuffer size in pixels (retina-correct); updated from the
	// framebuffer-size callback.
	Vector2 GetSize() const;
	// GLFW content / window size in screen coordinates (matches cursor pos).
	Vector2 GetContentSize() const;
	void SetSize(i32 width, i32 height);
	void SetTitle(std::string_view title);
	// Decodes an image file (PNG, JPEG, ...) and uses it as the window icon.
	// GLFW ignores window icons on macOS and Wayland. Returns false (logged)
	// if the bytes are not a decodable image.
	bool SetIcon(const std::byte *data, std::size_t size);
	// Empty or non-text clipboard contents return an empty string without logging an error.
	std::string GetClipboard() const;
	void SetClipboard(const std::string &text);

	void PollEvents();
	void SwapBuffers();

	GLFWwindow *Handle() const {
		return _window;
	}

	void SetInput(Input *input) {
		_input = input;
	}

	Input *GetInput() const {
		return _input;
	}

  private:
	static void ErrorCallback(int error, const char *description);
	static void FramebufferSizeCallback(GLFWwindow *window, int width, int height);
	static void KeyCallback(GLFWwindow *window, int key, int scancode, int action, int mods);
	static void MouseButtonCallback(GLFWwindow *window, int button, int action, int mods);
	static void CursorPosCallback(GLFWwindow *window, double x, double y);
	static void ScrollCallback(GLFWwindow *window, double xOffset, double yOffset);
	static void CharCallback(GLFWwindow *window, unsigned int codepoint);

	void onFramebufferResize(i32 width, i32 height);

	GLFWwindow *_window = nullptr;
	Input *_input = nullptr;
	i32 _framebufferWidth = 0;
	i32 _framebufferHeight = 0;
};

} // namespace sowa
