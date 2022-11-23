#include "Utils/File.hpp"

#include "Utils/String.hpp"
#include <fstream>
#include <iostream>
#include <random>
#include <string.h>
#include <unordered_map>

namespace Ease {
namespace File {
// res:// -> res
std::unordered_map<std::string, std::filesystem::path> File_path_endpoints;

std::filesystem::path Path(const std::filesystem::path &path, FilePathData *pathdata) {
	std::vector<std::string> tokens = Ease::Split(path.string(), "://");
	if (tokens.size() != 2) {
		std::cout << "Invalid Path: " << path << std::endl;
		return "";
	}
	std::string endpoint = tokens[0];
	std::filesystem::path relativePath = tokens[1];

	if (File_path_endpoints.count(endpoint) == 0) {
		std::cout << "Endpoint " << tokens[0] << " not found: " << path << std::endl;
		return "";
	}
	std::filesystem::path realPath = File_path_endpoints[endpoint] / relativePath;
	if (pathdata) {
		pathdata->endpoint = endpoint;
		pathdata->relativePath = relativePath;
	}
	return realPath;
}

bool InsertFilepathEndpoint(const std::string &endpoint, const std::filesystem::path &path, bool force) {
	if (!force && File_path_endpoints.count(endpoint) != 0)
		return false;

	File_path_endpoints[endpoint] = path;
	return true;
}

// todo: load from archive
std::vector<unsigned char> GetFileContent(const char *path) {
	try {
		std::filesystem::path fullpath = File::Path(path);

		std::ifstream ifstream(fullpath, std::ifstream::binary);
		ifstream.unsetf(std::ios::skipws);

		std::streampos fileSize;
		ifstream.seekg(0, std::ios::end);
		fileSize = ifstream.tellg();
		ifstream.seekg(0, std::ios::beg);

		if (fileSize > 0) {
			std::vector<unsigned char> buffer(fileSize);
			ifstream.read((char *)buffer.data(), fileSize);
			return buffer;
		} else {
			throw std::runtime_error(std::string("File not found: ") + path);
		}
	} catch (const std::exception &e) {
		std::cerr << e.what() << '\n';
		return std::vector<unsigned char>({'\0'});
	}
}

std::filesystem::path CreateTempFile(unsigned char *data, size_t size) {
	if (!std::filesystem::is_directory(std::filesystem::temp_directory_path() / std::filesystem::path("swtmp"))) {
		std::filesystem::create_directory(std::filesystem::temp_directory_path() / std::filesystem::path("swtmp"));
	}

	const char *letters = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwyz";
	char filebuf[16] = {};
	for (int i = 0; i < 16; i++)
		filebuf[i] = letters[rand() % strlen(letters)];

	std::filesystem::path path =
		std::filesystem::temp_directory_path() /
		std::filesystem::path("swtmp") /
		std::string(filebuf, 16);

	std::ofstream ofstream;
	ofstream.open(path, std::ios::binary | std::ios::out);
	ofstream.write(reinterpret_cast<const char *>(data), size);
	ofstream.close();

	return path;
}
} // namespace File
} // namespace Ease