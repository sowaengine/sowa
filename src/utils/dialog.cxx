#include "utils.hxx"

#include "tinyfiledialogs.h"

std::filesystem::path Utils::OpenFileDialog(const char *title, std::vector<const char *> patterns, const char *patternName, bool multiselect) {

	char *data = tinyfd_openFileDialog(title, "", patterns.size(), patterns.data(), patternName, multiselect ? 1 : 0);
	if (data == NULL)
		return std::filesystem::path("");

	std::filesystem::path ret = std::filesystem::path(std::string(data));

	return ret;
}