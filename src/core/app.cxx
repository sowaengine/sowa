#include "app.hxx"

#include "core/graphics.hxx"
#include "servers/input_server.hxx"
#include "servers/rendering_server.hxx"

#ifdef SW_WEB
#include <emscripten.h>
#endif

Error App::Init() {
	RenderingServer::GetInstance().CreateWindow(800, 600, "Sowa Engine");

	// Initialize rendering
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

	return OK;
}

Error App::Run() {
#ifdef SW_WEB
	emscripten_set_main_loop_arg(mainLoopCaller, this, 0, 1);
#else
	while (!RenderingServer::GetInstance().WindowShouldClose()) {
		mainLoop();
	}
#endif
	return OK;
}

void App::mainLoop() {
	InputServer::GetInstance().ProcessInput();
	glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	mainShader.Bind();
	rectModel.Draw();

	RenderingServer::GetInstance().SwapBuffers();
	InputServer::GetInstance().PollEvents();
}

void App::mainLoopCaller(void *self) {
	static_cast<App *>(self)->mainLoop();
}