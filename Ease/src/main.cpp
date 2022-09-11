#include "Debug.hpp"
#include <iostream>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

#include "Core/Application.hpp"
#include "spdlog/sinks/basic_file_sink.h"

int main(int argc, char const *argv[]) {
	auto logger = spdlog::basic_logger_mt("Ease", "logs/ease.log");
	spdlog::flush_on(spdlog::level::info);

	Ease::Application &app = Ease::Application::get_singleton();

	try {
		app.Run(argc, argv);
	} catch (const std::exception &e) {
		ERROR("{}", e.what());
		std::cerr << e.what() << '\n';
	}
	return 0;
}
