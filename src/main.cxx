#include <fstream>
#include <iostream>
#include <vector>

#include "data/project_settings.hxx"

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

	in vec3 aPos;
	// layout(location = 0) in vec3 aPos;
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

int main() {
	RenderingServer::GetInstance().CreateWindow(800, 600, "Sowa Engine");

	unsigned int VBO;
	glGenBuffers(1, &VBO);

	unsigned int EBO;
	glGenBuffers(1, &EBO);

	unsigned int vertexShader;
	vertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
	glCompileShader(vertexShader);
	{
		int success;
		char infoLog[512];
		glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);

		if (!success) {
			glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
			std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n"
					  << infoLog << std::endl;
		}
	}

	unsigned int fragmentShader;
	fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
	glCompileShader(fragmentShader);
	{
		int success;
		char infoLog[512];
		glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);

		if (!success) {
			glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
			std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n"
					  << infoLog << std::endl;
		}
	}

	shaderProgram = glCreateProgram();

	glAttachShader(shaderProgram, vertexShader);
	glAttachShader(shaderProgram, fragmentShader);
	glLinkProgram(shaderProgram);
	{
		int success;
		char infoLog[512];
		glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);

		if (!success) {
			glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
			std::cout << "ERROR::SHADER::PROGRAM::COMPILATION_FAILED\n"
					  << infoLog << std::endl;
		}
	}
	glGenVertexArrays(1, &VAO);
	glBindVertexArray(VAO);

	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)0);
	int loc = glGetAttribLocation(shaderProgram, "aPos");
	glEnableVertexAttribArray(loc);

#ifdef SW_WEB
	emscripten_set_main_loop(mainLoop, 0, 1);
#else
	while (!RenderingServer::GetInstance().WindowShouldClose()) {
		mainLoop();
	}
#endif

	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);

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

	glUseProgram(shaderProgram);
	glBindVertexArray(VAO);
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

	RenderingServer::GetInstance().SwapBuffers();
	InputServer::GetInstance().PollEvents();
}

extern "C" void Resize(int width, int height) {
}