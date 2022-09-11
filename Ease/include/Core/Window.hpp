#ifndef _E_WINDOW_HPP__
#define _E_WINDOW_HPP__
#pragma once

#include "nmGfx/src/Core/Window.hpp"
#include <string>

namespace Ease {
class Window {
  public:
	Window();
	~Window();

	/**
	 * @brief Video Size : Render resolution of scene, different than window size
	 *
	 * @return int
	 */
	inline int GetVideoWidth() { return _windowHandle->GetVideoWidth(); }
	/**
	 * @brief Video Size : Render resolution of scene, different than window size
	 *
	 * @return int
	 */
	inline int GetVideoHeight() { return _windowHandle->GetVideoHeight(); }

	inline int GetWindowWidth() { return _windowHandle->GetWindowWidth(); }
	inline int GetWindowHeight() { return _windowHandle->GetWindowHeight(); }

	bool ShouldClose();

	int GetBlackbarWidth();
	int GetBlackbarHeight();

#ifdef EASE_EDITOR
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

	nmGfx::Window *_windowHandle{nullptr};
};
} // namespace Ease

#endif