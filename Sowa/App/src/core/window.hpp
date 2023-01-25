#ifndef _E_WINDOW_HPP__
#define _E_WINDOW_HPP__
#pragma once

#include "stlpch.hpp"

#include "core/input.hpp"
#include "sowa.hpp"
#include "utils/math.hpp"

#include "resource/texture/texture.hpp"

namespace nmGfx {
class Window;
}

namespace Sowa {

class EngineContext;

/// @brief implemented in Window.cpp, used for accessing and modifying internal window data
class WindowAccessor;

enum class KeyState : uint32_t {
	UP = 0,	  // key is up
	DOWN,	  // key is down
	PRESSED,  // key just pressed
	RELEASED, // key just released
};

class Window {
  public:
	Window();
	~Window();

	void InitWindow(nmGfx::Window &window, EngineContext &ctx);
	void UpdateEvents(); // call before polling glfw events

	/* -- Input -- */
	Vector2 GetMousePosition();
	Vector2 GetGameMousePosition();

	bool IsKeyJustPressed(int key);
	bool IsKeyJustReleased(int key);
	bool IsKeyDown(int key);
	bool IsKeyUp(int key);

	double GetScrollDeltaY();
	double GetScrollDeltaX();

	/**
	 * @brief Video Size : Render resolution of scene, different than window size
	 *
	 * @return int
	 */
	int GetVideoWidth();
	/**
	 * @brief Video Size : Render resolution of scene, different than window size
	 *
	 * @return int
	 */
	int GetVideoHeight();

	int GetWindowWidth();
	int GetWindowHeight();

	bool ShouldClose();

	bool SetWindowIcon(Reference<ImageTexture> icon);

	int GetBlackbarWidth();
	int GetBlackbarHeight();

#ifdef SW_EDITOR
  public:
	void Editor_SetBlackbarWidth(int width) { m_Editor_BlackbarWidth = width; }
	void Editor_SetBlackbarHeight(int height) { m_Editor_BlackbarHeight = height; }

	void Editor_SetWindowWidth(int width) { m_Editor_WindowWidth = width; }
	void Editor_SetWindowHeight(int height) { m_Editor_WindowHeight = height; }

	inline int Editor_GetWindowWidth() { return m_Editor_WindowWidth; }
	inline int Editor_GetWindowHeight() { return m_Editor_WindowHeight; }

	void Editor_SetWindowPos(int x, int y) {
		m_Editor_WindowPosX = x;
		m_Editor_WindowPosY = y;
	}
	int Editor_GetWindowPosX() { return m_Editor_WindowPosX; }
	int Editor_GetWindowPosY() { return m_Editor_WindowPosY; }

  private:
	int m_Editor_BlackbarWidth;
	int m_Editor_BlackbarHeight;

	int m_Editor_WindowWidth;
	int m_Editor_WindowHeight;

	int m_Editor_WindowPosX;
	int m_Editor_WindowPosY;
#endif
  private:
	friend class Application;
	friend class WindowAccessor;

	nmGfx::Window *_windowHandle{nullptr};
	EngineContext *_Ctx{nullptr};

	std::map<int, KeyState> _KeyStates{};
	double _ScrollDeltaY{0.0};
	double _ScrollDeltaX{0.0};
};
} // namespace Sowa

#endif