#include <fstream>
#include <iostream>
#include <vector>

#include "data/project_settings.hxx"

#define GLFW_INCLUDE_NONE

#ifdef SW_WEB
#include <GLES3/gl3.h>
#include <GLFW/glfw3.h>
#include <emscripten.h>
#else
#include <glad/glad.h>
//
#include <GLFW/glfw3.h>
#endif

#include "servers/input_server.hxx"
#include "servers/rendering_server.hxx"

#include "core/rendering/model.hxx"
#include "core/rendering/model_builder.hxx"
#include "core/rendering/shader.hxx"

float vertices[] = {
	0.5f, 0.5f, 0.0f,	// top right
	0.5f, -0.5f, 0.0f,	// bottom right
	-0.5f, -0.5f, 0.0f, // bottom left
	-0.5f, 0.5f, 0.0f	// top left
};
unsigned int indices[] = {
	0, 1, 3,
	1, 2, 3};

#define GLSL(x) "#version 300 es\n" #x

const char *vertexShaderSource = GLSL(
	precision mediump float;

	layout(location = 0) in vec3 aPos;
	void main() {
		gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);
	});

const char *fragmentShaderSource = GLSL(
	precision mediump float;
	out vec4 FragColor;

	void main() {
		FragColor = vec4(1.0f, 0.5f, 0.2f, 1.0f);
		// gl_FragColor = vec4(1.0f, 0.5f, 0.2f, 1.0f);
	});

void mainLoop();

unsigned int VAO;
unsigned int shaderProgram;

Model rectModel;
Shader mainShader;

int main() {
	RenderingServer::GetInstance().CreateWindow(800, 600, "Sowa Engine");

	ModelBuilder::Quad2D(rectModel);

	mainShader.SetVertexSource(GLSL(
		precision mediump float;

		layout(location = 0) in vec3 aPos;
		void main() {
			gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);
		}));
	mainShader.SetFragmentSource(GLSL(
		precision mediump float;
		out vec4 FragColor;

		void main() {
			FragColor = vec4(1.0f, 0.6f, 0.2f, 1.0f);
			// gl_FragColor = vec4(1.0f, 0.5f, 0.2f, 1.0f);
		}));
	mainShader.Build();

#ifdef SW_WEB
	emscripten_set_main_loop(mainLoop, 0, 1);
#else
	while (!RenderingServer::GetInstance().WindowShouldClose()) {
		mainLoop();
	}
#endif

	rectModel.Delete();
	mainShader.Delete();

	glfwTerminate();
	return 0;
}

void framebuffer_size_callback(GLFWwindow *window, int width, int height) {
	glViewport(0, 0, width, height);
}

void mainLoop() {
	InputServer::GetInstance().ProcessInput();
	glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	mainShader.Bind();
	rectModel.Draw();

	RenderingServer::GetInstance().SwapBuffers();
	InputServer::GetInstance().PollEvents();
}

extern "C" void Resize(int width, int height) {
}