#include "debug.hpp"

#include "core/application.hpp"

int main(int argc, char const *argv[]) {
	SW_ENTRY()
	Debug::Streams::GetInstance();

	std::vector<std::string> args;
	for (int i = 0; i < argc; i++)
		args.push_back(argv[i]);

	Sowa::Application &app = Sowa::Application::get_singleton();
	app.Init(args);

	try {
		while (app.Process())
			;
	} catch (const std::exception &e) {
		Debug::Error("{}", e.what());
	}

	return 0;
}

#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"