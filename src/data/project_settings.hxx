#ifndef SW_DATA_PROJECT_SETTINGS_HXX
#define SW_DATA_PROJECT_SETTINGS_HXX
#pragma once

#include "data/size.hxx"
#include <string>

struct project_settings {
	std::string name = "";
	size window_size = size(1280, 720);
};

#endif // SW_DATA_PROJECT_SETTINGS_HXX