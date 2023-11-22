#include "utils.hxx"

#include "tinyfiledialogs.h"

std::filesystem::path Utils::OpenFileDialog(const char *title, std::vector<std::string> patterns, const char *patternName, bool multiselect) {
	char const *ps[patterns.size()];
	for (size_t i = 0; i < patterns.size(); i++) {
		ps[i] = patterns[i].c_str();
	}

	char *data = tinyfd_openFileDialog(title, "", patterns.size(), ps, patternName, multiselect ? 1 : 0);
	if (data == NULL)
		return std::filesystem::path("");

	std::filesystem::path ret = std::filesystem::path(std::string(data));

	return ret;
}