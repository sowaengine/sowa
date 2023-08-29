#include "input_server.hxx"
#include "input.hxx"
#include "rendering_server.hxx"

InputServer::InputServer() {
}
InputServer &InputServer::GetInstance() {
	static InputServer *server;
	return *server;
}

void InputServer::ProcessInput() {
	if (glfwGetKey(RenderingServer::GetInstance().m_pWindowHandle, KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(RenderingServer::GetInstance().m_pWindowHandle, true);
}

void InputServer::PollEvents() {
	glfwPollEvents();
}

void InputServer::GetMousePosition(double &x, double &y) {
	int w, h;
	RenderingServer::GetInstance().GetWindowSize(w, h);
	glfwGetCursorPos(RenderingServer::GetInstance().m_pWindowHandle, &x, &y);
	y = h - y;
}

bool InputServer::IsKeyDown(int key) {
	return glfwGetKey(RenderingServer::GetInstance().m_pWindowHandle, key) == GLFW_PRESS;
}