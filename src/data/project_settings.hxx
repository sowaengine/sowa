#ifndef SW_DATA_PROJECT_SETTINGS_HXX
#define SW_DATA_PROJECT_SETTINGS_HXX
#pragma once

#include <string>

#include "core/error/error.hxx"
#include "data/size.hxx"

struct project_settings {
	ErrorCode Load(const char *path);
	ErrorCode Save(const char *path);

	std::string app_name = "Sowa Engine";
	std::string app_version = "1.0";
	std::string app_author = "Author";

	std::string config_icon = "";
	std::string config_main_scene = "";

	int window_width = 1920;
	int window_height = 1080;

  private:
};

#endif // SW_DATA_PROJECT_SETTINGS_HXX