#include "project_settings.hxx"

#include <iostream>

#include "servers/file_server.hxx"
#include "utils/utils.hxx"

#include "google/protobuf/text_format.h"
#include "proto/project_settings.pb.h"

ErrorCode project_settings::Load(const char *path) {
	pb::ProjectSettings settings;

	Result<file_buffer *> buf = FileServer::get().load_file(path, ReadWriteFlags_FullPath | ReadWriteFlags_AsText);
	if (!buf.ok()) {
		return buf.error();
	}

	std::string v((char *)buf.get()->data(), buf.get()->size());
	google::protobuf::TextFormat::ParseFromString(v, &settings);

	app_author = settings.app().author();
	app_name = settings.app().name();
	app_version = settings.app().version();

	config_icon = settings.config().icon();
	config_main_scene = settings.config().main_scene();

	window_width = settings.window().width();
	window_height = settings.window().height();

	return OK;
}

ErrorCode project_settings::Save(const char *path) {
	pb::ProjectSettings settings;
	settings.mutable_app()->set_author(app_author);
	settings.mutable_app()->set_name(app_name);
	settings.mutable_app()->set_version(app_version);

	settings.mutable_config()->set_icon(config_icon);
	settings.mutable_config()->set_main_scene(config_main_scene);

	settings.mutable_window()->set_width(window_width);
	settings.mutable_window()->set_height(window_height);

	std::string str;
	if (!google::protobuf::TextFormat::PrintToString(settings, &str)) {
		Utils::Error("Failed to generate project settings file");
		return ERR_FAILED;
	}

	return FileServer::get().save_file(str.data(), str.size(), path, ReadWriteFlags_FullPath);
}
