#include "utils.hxx"

std::vector<std::string> Utils::Split(std::string str, std::string delimiter) {
	std::vector<std::string> tokens;

	size_t pos = 0;
	std::string token;
	while ((pos = str.find(delimiter)) != std::string::npos) {
		token = str.substr(0, pos);
		tokens.push_back(token);
		str.erase(0, pos + delimiter.length());
	}
	if (str != "")
		tokens.push_back(str);

	return tokens;
}