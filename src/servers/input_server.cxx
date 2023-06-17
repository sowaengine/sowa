#include "input_server.hxx"
#include "rendering_server.hxx"

InputServer::InputServer() {
}
InputServer &InputServer::GetInstance() {
	static InputServer *server;
	return *server;
}

void InputServer::ProcessInput() {
	if (glfwGetKey(RenderingServer::GetInstance().m_pWindowHandle, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(RenderingServer::GetInstance().m_pWindowHandle, true);
}

void InputServer::PollEvents() {
	glfwPollEvents();
}