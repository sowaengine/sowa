#include "project_settings.hxx"

#include <iostream>

#include "servers/file_server.hxx"

ErrorCode project_settings::Load(const char *path) {
	ErrorCode err = m_doc.LoadFile(path);
	if (err != OK) {
		return err;
	}

	int config_version = m_doc.Value("config_version", -1);
	if (config_version == 1) {
		return loadVersion1(m_doc);
	}

	return ERR_FAILED;
}

ErrorCode project_settings::Save(const char *path) {
	toml_document doc;

	toml_document app;
	app.Set("name", app_name)
		.Set("version", app_version)
		.Set("author", app_author);

	toml_document config;
	config.Set("icon", config_icon);

	toml_document window;
	window.Set("width", window_width)
		.Set("height", window_height);

	doc.Set("config_version", 1);
	doc.Set("app", app);
	doc.Set("config", config);
	doc.Set("window", window);

	std::string s;
	doc.Serialize(s);

	ErrorCode err = FileServer::get().WriteFileString(path, s);
	if (err != OK) {
		return err;
	}

	return OK;
}

ErrorCode project_settings::loadVersion1(toml_document &doc) {
	toml_document app = doc["app"];
	app_name = app.Value("name", app_name);
	app_version = app.Value("version", app_version);
	app_author = app.Value("author", app_author);

	toml_document config = doc["config"];
	config_icon = config.Value("icon", config_icon);

	toml_document window = doc["window"];
	window_width = window.Value("width", window_width);
	window_height = window.Value("height", window_height);

	return OK;
}