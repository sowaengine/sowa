#ifndef SW_RENDERING_SERVER_HXX
#define SW_RENDERING_SERVER_HXX
#pragma once

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <string>

#include "math/math.hxx"

enum class CursorStyle {
	Normal,
	Pointer,
	ResizeX,
	ResizeY,
	Resize
};

enum class CursorMode {
	Normal,
	Hidden,
	Disabled,
	Tiled
};

class RenderingServer {
  public:
	RenderingServer();

	static RenderingServer &get();
	void CreateWindow(int width, int height, const std::string &title);

	bool WindowShouldClose();
	void SwapBuffers();
	void GetWindowSize(int &width, int &height);
	vec2 GetWindowSize();

	void SetCursorStyle(CursorStyle style = CursorStyle::Normal);
	void SetCursorMode(CursorMode mode = CursorMode::Normal);
	inline CursorMode GetCursorMode() { return m_currentCursorMode; }

	void Terminate();

	inline bool Active() { return m_serverActive; }

  private:
	friend class WindowCallbackBridge;
	friend class InputServer;
	friend class GuiServer;
	GLFWwindow *m_pWindowHandle = nullptr;

	GLFWcursor *m_pStandartCursor = nullptr;
	GLFWcursor *m_pPointerCursor = nullptr;
	GLFWcursor *m_pResizeXCursor = nullptr;
	GLFWcursor *m_pResizeYCursor = nullptr;
	GLFWcursor *m_pResizeCursor = nullptr;

	CursorMode m_currentCursorMode = CursorMode::Normal;

	int m_windowWidth = 0;
	int m_windowHeight = 0;

	void framebuffer_size_callback(GLFWwindow *window, int width, int height);

	bool m_serverActive = false;
};

#endif // SW_RENDERING_SERVER_HXX