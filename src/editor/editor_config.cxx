#include "editor_config.hxx"

#include <filesystem>

#include "servers/file_server.hxx"
#include "utils/utils.hxx"

#ifndef SW_LINUX
#define SW_LINUX
#endif

#ifdef SW_LINUX
#include <pwd.h>
#include <sys/types.h>
#include <unistd.h>
#endif

#ifdef SW_WINDOWS
#include <shlobj.h>
#include <windows.h>
#endif

/*
  LINUX: ~/.local/config
*/

#ifdef SW_LINUX
static std::filesystem::path get_config_path() {
	const char *homeDir;

	homeDir = getenv("XDG_DATA_HOME"); // ~/.local/share
	if (homeDir == NULL) {
		homeDir = getenv("HOME");

		if (homeDir == NULL) {
			homeDir = getpwuid(getuid())->pw_dir;
		}
	}

	if (homeDir == NULL)
		return "";

	std::filesystem::path configPath = homeDir;
	configPath = configPath / ".sowa";

	if (std::filesystem::exists(configPath)) {
		if (!std::filesystem::is_directory(configPath)) {
			Utils::Error("{} must be a directory", configPath.string());
			return "";
		}
	} else {
		std::filesystem::create_directories(configPath);
	}

	return configPath / "config.toml";
}
#else
#error "Unknown platform to get editor config path"
#endif

EditorConfig &EditorConfig::get() {
	static EditorConfig *config = new EditorConfig;
	return *config;
}

ErrorCode EditorConfig::Load() {
	std::filesystem::path configPath = get_config_path();
	Utils::Info("Config path: {}", configPath.string());

	ErrorCode err = m_doc.LoadFile(configPath.c_str(), ReadWriteFlags_FullPath);
	if (err != OK) {
		return err;
	}

	int config_version = m_doc.Value("config_version", -1);
	if (config_version == 1) {
		return loadVersion1(m_doc);
	}

	return ERR_FAILED;
}

ErrorCode EditorConfig::Save() {
	toml_document doc;

	std::filesystem::path configPath = get_config_path();

	toml_document config;
	config.Set("last_project", last_project)
		.Set("auto_open_last_project", auto_open_last_project);

	doc.Set("config_version", 1);
	doc.Set("config", config);

	std::string s;
	doc.Serialize(s);

	ErrorCode err = FileServer::get().save_file(s.data(), s.size(), configPath.c_str(), ReadWriteFlags_FullPath);
	if (err != OK) {
		return err;
	}

	return OK;
}

ErrorCode EditorConfig::loadVersion1(toml_document &doc) {
	toml_document config = doc["config"];
	last_project = config.Value("last_project", last_project);
	auto_open_last_project = config.Value("auto_open_last_project", auto_open_last_project);

	return OK;
}