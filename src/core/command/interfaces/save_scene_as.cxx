#include "scene_save_as.hxx"

#include <iostream>
#include <string>

#include "core/app.hxx"

SceneSaveAsInterface::SceneSaveAsInterface(std::string path) {
	if (path == "") {
		text = "res://scene.escn";
	} else {
		text = path;
	}
	text_input = true;
	text_cursor = text.size();

	action = [this]() {
		Error err = App::GetInstance().GetCurrentScene()->Save(this->text.c_str());
		if (err != OK) {
			std::cout << "Failed to save scene " << err << std::endl;
		}
	};
}