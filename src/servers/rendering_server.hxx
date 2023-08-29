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
	friend struct CallbackBridge;
	friend class InputServer;
	GLFWwindow *m_pWindowHandle = nullptr;

	GLFWcursor *m_pStandartCursor = nullptr;
	GLFWcursor *m_pPointerCursor = nullptr;
	GLFWcursor *m_pResizeXCursor = nullptr;
	GLFWcursor *m_pResizeYCursor = nullptr;
	GLFWcursor *m_pResizeCursor = nullptr;

	double m_input_mouseX = 0.f;
	double m_input_mouseY = 0.f;

	void framebuffer_size_callback(GLFWwindow *window, int width, int height);
	void mouse_button_callback(GLFWwindow *window, int button, int action, int pressed);
	void cursor_pos_callback(GLFWwindow *window, double x, double y);
	void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods);
	void char_callback(GLFWwindow *window, unsigned int codepoint);

	bool m_serverActive = false;
};

#endif // SW_RENDERING_SERVER_HXX