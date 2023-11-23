#include "scene_save_as.hxx"

#include <iostream>
#include <string>

#include "core/app.hxx"
#include "core/error.hxx"

SceneSaveAsInterface::SceneSaveAsInterface(std::string path) {
	if (path == "") {
		text = "res://scene.escn";
	} else {
		text = path;
	}
	text_input = true;
	text_cursor = text.size();

	std::filesystem::path p = text;
	if (p.has_extension()) {
		text_cursor -= p.extension().string().size();
	}

	action = [this]() {
		ErrorCode err = App::get().GetCurrentScene()->save(text.c_str());
		if (err != OK) {
			std::cout << "Failed to save scene " << err << std::endl;
		}
	};
}