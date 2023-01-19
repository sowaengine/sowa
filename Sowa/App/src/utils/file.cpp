#include "utils/file.hpp"

#include "debug.hpp"
#include "utils/string.hpp"

#ifdef SW_LINUX
#include <pwd.h>
#include <sys/types.h>
#include <unistd.h>
#endif

#ifdef SW_WINDOWS
#include <shlobj.h>
#include <windows.h>
#endif

namespace Sowa {
namespace File {
// res:// -> res
std::unordered_map<std::string, std::filesystem::path> File_path_endpoints;

std::filesystem::path Path(const std::filesystem::path &path, FilePathData *pathdata) {
	std::vector<std::string> tokens = Sowa::Split(path.string(), "://");
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

#if defined(SW_LINUX)
bool RegisterDataPath() {
	// ~/.local/share/Sowa Engine/
	const char *homeDir;
	bool isHome = false; // is homeDir ~/ or ~/.local/share

	homeDir = getenv("XDG_DATA_HOME"); // ~/.local/share
	if (homeDir == NULL) {
		isHome = true;
		homeDir = getenv("HOME"); // ~/

		if (homeDir == NULL) {
			homeDir = getpwuid(getuid())->pw_dir; // ~/
		}
	}

	if (homeDir == NULL) {
		return false;
	}

	std::filesystem::path homePath = homeDir;
	if (isHome) {
		homePath = homePath / ".local";
		homePath = homePath / "share";
	}

	std::filesystem::path dataPath = homePath / "Sowa Engine";
	if (std::filesystem::exists(dataPath)) {
		if (!std::filesystem::is_directory(dataPath)) {
			Debug::Error("~/.local/share/Sowa Engine is a file. Must be a directory");
			return false;
		}
	} else {
		std::filesystem::create_directories(dataPath);
	}

	std::filesystem::path usrPath = dataPath / "usr";
	if (std::filesystem::exists(usrPath)) {
		if (!std::filesystem::is_directory(usrPath)) {
			Debug::Error("usr:// path must be a directory");
			return false;
		}
	} else {
		std::filesystem::create_directories(usrPath);
	}

	std::filesystem::path editorPath = dataPath / "editor";
	if (std::filesystem::exists(editorPath)) {
		if (!std::filesystem::is_directory(editorPath)) {
			Debug::Error("editor:// path must be a directory");
			return false;
		}
	} else {
		std::filesystem::create_directories(editorPath);
	}

	InsertFilepathEndpoint("data", dataPath, true);
	InsertFilepathEndpoint("usr", usrPath, true);
	InsertFilepathEndpoint("editor", editorPath, true);
	return true;
}
#elif defined(SW_WINDOWS)
bool RegisterDataPath() {
	// C:\Users\USER\Local Settings\Application Data\Sowa Engine
	TCHAR szPath[MAX_PATH];
	if (SUCCEEDED(SHGetFolderPath(NULL, CSIDL_LOCAL_APPDATA | CSIDL_FLAG_CREATE, NULL, 0, szPath))) {
		std::filesystem::path homePath = szPath;

		std::filesystem::path dataPath = homePath / "Sowa Engine";
		if (std::filesystem::exists(dataPath)) {
			if (!std::filesystem::is_directory(dataPath)) {
				Debug::Error("APPDATA/Sowa Engine is a file. Must be a directory");
				return false;
			}
		} else {
			std::filesystem::create_directories(dataPath);
		}

		std::filesystem::path usrPath = dataPath / "usr";
		if (std::filesystem::exists(usrPath)) {
			if (!std::filesystem::is_directory(usrPath)) {
				Debug::Error("usr:// path must be a directory");
				return false;
			}
		} else {
			std::filesystem::create_directories(usrPath);
		}

		std::filesystem::path editorPath = dataPath / "editor";
		if (std::filesystem::exists(editorPath)) {
			if (!std::filesystem::is_directory(editorPath)) {
				Debug::Error("editor:// path must be a directory");
				return false;
			}
		} else {
			std::filesystem::create_directories(editorPath);
		}

		InsertFilepathEndpoint("data", dataPath, true);
		InsertFilepathEndpoint("usr", usrPath, true);
		InsertFilepathEndpoint("editor", editorPath, true);
		return true;
	}
	return false;
}
#else
#error "Sowa::File::RegisterDataPath() is not implemented in current platform"
#endif

} // namespace File
} // namespace Sowa