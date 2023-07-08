#define GLFW_INCLUDE_NONE

#include "rendering_server.hxx"

#include "core/app.hxx"
#include "core/rendering/gl.hxx"

#include <iostream>
#include <stdexcept>
#include <stdio.h>

struct CallbackBridge {
	CallbackBridge() = default;

	void FramebufferSizeCallback(GLFWwindow *window, int width, int height) {
		RenderingServer::GetInstance().framebuffer_size_callback(window, width, height);
	}

	void MouseButtonCallback(GLFWwindow *window, int button, int action, int mods) {
		RenderingServer::GetInstance().mouse_button_callback(window, button, action, mods);
	}

	void CursorPosCalllback(GLFWwindow *window, double x, double y) {
		RenderingServer::GetInstance().cursor_pos_callback(window, x, y);
	}
};

static void CallbackWrapperFramebufferSizeCallback(GLFWwindow *window, int width, int height) {
	CallbackBridge().FramebufferSizeCallback(window, width, height);
}

static void CallbackWrapperMouseButtonCallback(GLFWwindow *window, int button, int action, int mods) {
	CallbackBridge().MouseButtonCallback(window, button, action, mods);
}

static void CallbackWrapperCursorPosCallback(GLFWwindow *window, double x, double y) {
	CallbackBridge().CursorPosCalllback(window, x, y);
}

static void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods) {
	if (action == GLFW_PRESS) {
	}
}

static void char_callback(GLFWwindow *window, unsigned int codepoint) {
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
}

RenderingServer &RenderingServer::GetInstance() {
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

#ifndef SW_WEB
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		throw std::runtime_error("Failed to initailize glad");
		return;
	}
#endif

	glfwSetFramebufferSizeCallback(m_pWindowHandle, CallbackWrapperFramebufferSizeCallback);
	glfwSetCharCallback(m_pWindowHandle, char_callback);
	glfwSetKeyCallback(m_pWindowHandle, key_callback);
	glfwSetMouseButtonCallback(m_pWindowHandle, CallbackWrapperMouseButtonCallback);
	glfwSetCursorPosCallback(m_pWindowHandle, CallbackWrapperCursorPosCallback);
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

void RenderingServer::SetCursorMode(CursorMode mode) {
	if (mode == CursorMode::Normal) {
		glfwSetCursor(m_pWindowHandle, m_pStandartCursor);
	} else if (mode == CursorMode::Pointer) {
		glfwSetCursor(m_pWindowHandle, m_pPointerCursor);
	} else if (mode == CursorMode::ResizeX) {
		glfwSetCursor(m_pWindowHandle, m_pResizeXCursor);
	} else if (mode == CursorMode::ResizeY) {
		glfwSetCursor(m_pWindowHandle, m_pResizeYCursor);
	} else if (mode == CursorMode::Resize) {
		glfwSetCursor(m_pWindowHandle, m_pResizeCursor);
	}
}

void RenderingServer::Terminate() {
	glfwTerminate();
}

void RenderingServer::framebuffer_size_callback(GLFWwindow *window, int width, int height) {
	glViewport(0, 0, width, height);
}

void RenderingServer::mouse_button_callback(GLFWwindow *window, int button, int action, int mods) {
	InputEventMouseButton event;
	static std::unordered_map<int, MouseButton> buttons = {
		{GLFW_MOUSE_BUTTON_LEFT, MB_LEFT},
		{GLFW_MOUSE_BUTTON_RIGHT, MB_RIGHT},
		{GLFW_MOUSE_BUTTON_MIDDLE, MB_MIDDLE},
		{GLFW_MOUSE_BUTTON_4, MB_4},
		{GLFW_MOUSE_BUTTON_5, MB_5},
		{GLFW_MOUSE_BUTTON_6, MB_6},
		{GLFW_MOUSE_BUTTON_7, MB_7},
		{GLFW_MOUSE_BUTTON_8, MB_8}};

	event.button = buttons[button];

	if (action == GLFW_PRESS)
		event.action = PRESSED;
	else if (action == GLFW_RELEASE)
		event.action = RELEASED;

	if (mods & GLFW_MOD_SHIFT)
		event.modifiers.shift = true;
	if (mods & GLFW_MOD_CONTROL)
		event.modifiers.control = true;
	if (mods & GLFW_MOD_ALT)
		event.modifiers.alt = true;
	if (mods & GLFW_MOD_SUPER)
		event.modifiers.super = true;

	App::GetInstance().MouseInputCallback()(event);
}

void RenderingServer::cursor_pos_callback(GLFWwindow *window, double x, double y) {
	InputEventMouseMove event;

	double deltaX = x - m_input_mouseX;
	double deltaY = y - m_input_mouseY;
	m_input_mouseX = x;
	m_input_mouseY = y;

	event.deltaX = deltaX;
	event.deltaY = deltaY;
	event.mouseX = x;
	event.mouseY = y;

	App::GetInstance().MouseMoveCallback()(event);
}