#ifndef SW_RENDERING_SERVER_HXX
#define SW_RENDERING_SERVER_HXX
#pragma once

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <string>

class RenderingServer {
  public:
	RenderingServer();

	static RenderingServer &GetInstance();
	void CreateWindow(int width, int height, const std::string &title);

	bool WindowShouldClose();
	void SwapBuffers();

	void Terminate();

  private:
	friend struct CallbackBridge;
	friend class InputServer;
	GLFWwindow *m_pWindowHandle = nullptr;

	void framebuffer_size_callback(GLFWwindow *window, int width, int height);
};

#endif // SW_RENDERING_SERVER_HXX