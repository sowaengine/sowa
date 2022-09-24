#include "Debug.hpp"
#include <iostream>

#include "Core/Application.hpp"

int main(int argc, char const *argv[]) {
	SW_ENTRY()
	Ease::Application &app = Ease::Application::get_singleton();

	Debug::Info("---> Sowa Engine v{} Init <---", EASE_VERSION_STRING);

	try {
		app.Run(argc, argv);
	} catch (const std::exception &e) {
		Debug::Error("{}", e.what());
	}

	Debug::Info("---> Sowa Engine v{} Exit <---\n\n", EASE_VERSION_STRING);
	return 0;
}
