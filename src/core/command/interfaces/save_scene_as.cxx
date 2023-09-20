#include "scene_save_as.hxx"

#include <iostream>
#include <string>

#include "core/app.hxx"

SceneSaveAsInterface::SceneSaveAsInterface(std::string path) {
	if (path == "") {
		text = L"res://scene.escn";
	} else {
		std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
		std::wstring wide_string = converter.from_bytes(path);

		text = wide_string;
	}
	text_input = true;
	text_cursor = text.size();

	std::filesystem::path p = text;
	if (p.has_extension()) {
		text_cursor -= p.extension().string().size();
	}

	action = [this]() {
		std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
		std::string str = converter.to_bytes(this->text);

		Error err = App::get().GetCurrentScene()->save(str.c_str());
		if (err != OK) {
			std::cout << "Failed to save scene " << err << std::endl;
		}
	};
}