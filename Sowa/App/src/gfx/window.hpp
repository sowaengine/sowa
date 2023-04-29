#ifndef SW_GFX_WINDOW_HPP_
#define SW_GFX_WINDOW_HPP_

#include <string>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include "core/engine_context.hpp"
#include "math/math.hpp"
#include "resource/texture/image_texture.hpp"
#include "sowa.hpp"
#include "./gl/texture_gl.hpp"

namespace sowa {
class Application;
namespace gfx {

typedef uint32_t WindowFlags;
enum WindowFlags_ : uint32_t {
	None = 0,
	Fullscreen = 1 << 1,
	Hidden = 1 << 2,
};

struct InitWindowArgs {
	int windowWidth;
	int windowHeight;
	int videoWidth;
	int videoHeight;
	std::string title;
	WindowFlags flags;

	InitWindowArgs() {
		videoWidth = 1920;
		videoHeight = 1080;
		windowWidth = 1280;
		windowHeight = 720;
		title = "Sowa Engine";
		flags = WindowFlags_::None;
	}
};

enum class KeyState : uint32_t {
	UP = 0,	  // key is up
	DOWN,	  // key is down
	PRESSED,  // key just pressed
	RELEASED, // key just released
};
enum class ButtonState : uint32_t {
	UP = 0,
	DOWN,
	PRESSED,
	RELEASED,
};

class Window {
  public:
	bool InitWindow(InitWindowArgs);
	void DestroyWindow();

	inline bool IsFullscreen() { return m_fullscreen; }
	void SetFullscreen(bool);

	void PollEvents();
	void SwapBuffers();
	bool ShouldClose();

	sowa::vec2 GetMousePosition();
	sowa::vec2 GetVideoMousePosition();

	bool IsKeyJustPressed(int key);
	bool IsKeyJustReleased(int key);
	bool IsKeyDown(int key);

	bool IsButtonJustPressed(int button);
	bool IsButtonJustReleased(int button);
	bool IsButtonDown(int button);

	double GetScrollDeltaY();
	double GetScrollDeltaX();

	inline const sowa::vec2 GetWindowSize() const { return m_windowsize; }
	inline const sowa::vec2 GetVideoSize() const { return m_videosize; }

	bool SetWindowIcon(GLTexture& icon);

	~Window();
	Window() = default;
  private:
	friend class sowa::Application;
	friend class WindowManager;

	Window(EngineContext* ctx);

	EngineContext *m_ctx{nullptr};

	static void ErrorCallback(int error, const char *description);
	static void FramebufferSizeCallback(GLFWwindow *window, int width, int height);
	static void KeyCallback(GLFWwindow *window, int key, int scancode, int action, int mods);
	static void ScrollCallback(GLFWwindow *window, double xOffset, double yOffset);
	static void MouseButtonCallback(GLFWwindow *window, int button, int action, int mods);
	static void CursorPosCallback(GLFWwindow *window, double xPos, double yPos);
	static void CharCallback(GLFWwindow *window, unsigned int codePoint);

	GLFWwindow *p_window{nullptr};
	bool m_fullscreen{false};
	sowa::vec2 m_windowsize;
	sowa::vec2 m_videosize;

	// Store old x, y, w, h to revert back after returning back to windowed mode
	int m_fullscreen_old_x = 0;
	int m_fullscreen_old_y = 0;
	int m_fullscreen_old_w = 0;
	int m_fullscreen_old_h = 0;
	int m_fullscreen_old_refresh = 0;

	// input management
	std::map<int, KeyState> m_keyStates{};
	std::map<int, ButtonState> m_buttonStates{};
	sowa::vec2 m_scrollDelta;
	sowa::vec2 m_mousePos;
};

} // namespace gfx
} // namespace sowa

#endif // SW_GFX_WINDOW_HPP_