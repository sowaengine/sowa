#include "debug.hpp"

#include "core/application.hpp"

int main(int argc, char const *argv[]) {
	SW_ENTRY()
	Sowa::Application &app = Sowa::Application::get_singleton();
	app.Init(argc, argv);

	Debug::Info("---> Sowa Engine v{} Init <---", SOWA_VERSION_STRING);

	try {
		while (app.Process())
			;
	} catch (const std::exception &e) {
		Debug::Error("{}", e.what());
	}

	Debug::Info("---> Sowa Engine v{} Exit <---\n\n", SOWA_VERSION_STRING);
	return 0;
}

#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"