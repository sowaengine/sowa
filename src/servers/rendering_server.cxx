#define GLFW_INCLUDE_NONE

#include "rendering_server.hxx"
#include "input_server.hxx"

#include "core/app.hxx"
#include "core/rendering/gl.hxx"

#include <iostream>
#include <stdexcept>
#include <stdio.h>

struct WindowCallbackBridge {
	WindowCallbackBridge() = default;

	void FramebufferSizeCallback(GLFWwindow *window, int width, int height) {
		RenderingServer::get().framebuffer_size_callback(window, width, height);
	}
};

static void CallbackWrapperFramebufferSizeCallback(GLFWwindow *window, int width, int height) {
	WindowCallbackBridge().FramebufferSizeCallback(window, width, height);
}

RenderingServer::RenderingServer() {
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	// glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

	m_pStandartCursor = glfwCreateStandardCursor(GLFW_ARROW_CURSOR);
	m_pPointerCursor = glfwCreateStandardCursor(GLFW_HAND_CURSOR);
	m_pResizeXCursor = glfwCreateStandardCursor(GLFW_HRESIZE_CURSOR);
	m_pResizeYCursor = glfwCreateStandardCursor(GLFW_VRESIZE_CURSOR);
	m_pResizeCursor = glfwCreateStandardCursor(GLFW_CROSSHAIR_CURSOR);

	m_serverActive = true;
}

RenderingServer &RenderingServer::get() {
	static RenderingServer *server = new RenderingServer;
	return *server;
}

void RenderingServer::CreateWindow(int width, int height, const std::string &title) {
	if (nullptr != m_pWindowHandle) {
		throw std::runtime_error("Failed to create window. Window already exists");
	}

	if (width <= 0) {
		width = 640;
	}
	if (height <= 0) {
		height = 480;
	}

	m_pWindowHandle = glfwCreateWindow(width, height, title.c_str(), NULL, NULL);
	if (m_pWindowHandle == NULL) {
		glfwTerminate();
		throw std::runtime_error("Failed to create GLFW window");
		return;
	}

	glfwMakeContextCurrent(m_pWindowHandle);
	// glfwSwapInterval(0);

#ifndef SW_WEB
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		throw std::runtime_error("Failed to initailize glad");
		return;
	}
#endif

	m_windowWidth = width;
	m_windowHeight = height;
	InputServer::get().initialize();

	glfwSetFramebufferSizeCallback(m_pWindowHandle, CallbackWrapperFramebufferSizeCallback);
}

bool RenderingServer::WindowShouldClose() {
	return glfwWindowShouldClose(m_pWindowHandle);
}

void RenderingServer::SwapBuffers() {
	glfwSwapBuffers(m_pWindowHandle);
}

void RenderingServer::GetWindowSize(int &width, int &height) {
	glfwGetFramebufferSize(m_pWindowHandle, &width, &height);
}

void RenderingServer::SetCursorStyle(CursorStyle style) {
	if (style == CursorStyle::Normal) {
		glfwSetCursor(m_pWindowHandle, m_pStandartCursor);
	} else if (style == CursorStyle::Pointer) {
		glfwSetCursor(m_pWindowHandle, m_pPointerCursor);
	} else if (style == CursorStyle::ResizeX) {
		glfwSetCursor(m_pWindowHandle, m_pResizeXCursor);
	} else if (style == CursorStyle::ResizeY) {
		glfwSetCursor(m_pWindowHandle, m_pResizeYCursor);
	} else if (style == CursorStyle::Resize) {
		glfwSetCursor(m_pWindowHandle, m_pResizeCursor);
	}
}

void RenderingServer::SetCursorMode(CursorMode mode) {
	if (mode == CursorMode::Normal) {
		glfwSetInputMode(m_pWindowHandle, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
		m_currentCursorMode = CursorMode::Normal;
	} else if (mode == CursorMode::Hidden) {
		glfwSetInputMode(m_pWindowHandle, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
		m_currentCursorMode = CursorMode::Hidden;
	} else if (mode == CursorMode::Disabled) {
		glfwSetInputMode(m_pWindowHandle, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
		m_currentCursorMode = CursorMode::Disabled;
	} else if (mode == CursorMode::Tiled) {
		glfwSetInputMode(m_pWindowHandle, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
		m_currentCursorMode = CursorMode::Tiled;
	}
}

void RenderingServer::Terminate() {
	m_serverActive = false;
	glfwTerminate();
}

void RenderingServer::framebuffer_size_callback(GLFWwindow *window, int width, int height) {
	App::get().WindowResizeCallback()(width, height);
}