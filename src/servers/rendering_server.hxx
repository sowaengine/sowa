#ifndef SW_RENDERING_SERVER_HXX
#define SW_RENDERING_SERVER_HXX
#pragma once

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <string>

enum class CursorMode {
	Normal,
	Pointer,
	ResizeX,
	ResizeY,
	Resize
};

class RenderingServer {
  public:
	RenderingServer();

	static RenderingServer &GetInstance();
	void CreateWindow(int width, int height, const std::string &title);

	bool WindowShouldClose();
	void SwapBuffers();
	void GetWindowSize(int &width, int &height);

	void SetCursorMode(CursorMode mode = CursorMode::Normal);

	void Terminate();

	inline bool Active() { return m_serverActive; }

  private:
	friend class WindowCallbackBridge;
	friend class InputServer;
	GLFWwindow *m_pWindowHandle = nullptr;

	GLFWcursor *m_pStandartCursor = nullptr;
	GLFWcursor *m_pPointerCursor = nullptr;
	GLFWcursor *m_pResizeXCursor = nullptr;
	GLFWcursor *m_pResizeYCursor = nullptr;
	GLFWcursor *m_pResizeCursor = nullptr;

	int m_windowWidth = 0;
	int m_windowHeight = 0;

	void framebuffer_size_callback(GLFWwindow *window, int width, int height);

	bool m_serverActive = false;
};

#endif // SW_RENDERING_SERVER_HXX