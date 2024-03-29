#include <fstream>
#include <iostream>
#include <vector>

#include "data/project_settings.hxx"

#include "servers/input_server.hxx"
#include "servers/rendering_server.hxx"

#include "core/app.hxx"

int main() {
	{ // App should be destructed before servers
		App app;
		app.Init();

		if (ErrorCode err = app.Run(); err != OK) {
			std::cerr << "Runtime error: " << err << std::endl;

			return err;
		}
	}

	RenderingServer::get().Terminate();
	return 0;
}