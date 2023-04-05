#include <fstream>
#include <iostream>
#include <string.h>

#include "console.hpp"
#include "net.hpp"
#include "utils.hpp"

#include "yaml-cpp/yaml.h"
#include "zip.h"

std::vector<std::string> Split(std::string text, const std::string &delimiter) {
	std::vector<std::string> tokens;

	size_t pos = 0;
	std::string token;
	while ((pos = text.find(delimiter)) != std::string::npos) {
		token = text.substr(0, pos);
		tokens.push_back(token);
		text.erase(0, pos + delimiter.length());
	}
	tokens.push_back(text);
	return tokens;
}

std::vector<std::string> GetAllVersions(std::string url) {
	std::string content = GetContentHTTP(url);
	std::cout << "Got Content: " << content << std::endl;
	return {};
}

std::vector<Version> ParseVersions(std::string yamlDoc) {
	std::vector<Version> versions{};

	YAML::Node node = YAML::Load(yamlDoc);
	YAML::Node vers = node["Versions"];
	if (vers) {
		for (YAML::const_iterator it = vers.begin(); it != vers.end(); ++it) {
			std::string key = it->first.as<std::string>("");
			YAML::Node value = it->second;

#if defined(SW_LINUX)
			std::string url = value["Url"]["Linux"].as<std::string>("");
#elif defined(SW_WINDOWS)
			std::string url = value["Url"]["Win64"].as<std::string>("");
#endif

			versions.push_back(Version(key, url));
		}
	}

	return versions;
}

/*
	1.0  -----------> 1.0.0-stable
	0.1.1-a1-65809 -> 0.1.1-a1
*/
std::string ResolveVersion(std::string version) {
	if(StartsWith(version, "sowa-")) {
		version = version.substr(strlen("sowa-"));
	}
	auto tokens = Split(version, ".");
	if (tokens.size() == 1) {
		if (IsInteger(version)) {
			return version + ".0.0-stable";
		} else if (version == "latest") {
			return version;
		}

		return "";
	}
	if (tokens.size() == 2) {
		return version + ".0-stable";
	}
	if (tokens.size() == 3) {
		auto rhsTokens = Split(tokens[2], "-");
		if (rhsTokens.size() == 1) {
			return version + "-stable";
		}
		if (rhsTokens.size() == 3) {
			//       0          .       1         .         1          .       a1
			return tokens[0] + "." + tokens[1] + "." + rhsTokens[0] + "-" + rhsTokens[1];
		}
	}

	return version;
}

BranchVersionPair ResolveFullVersion(std::string version) {
	std::string branch = "main";
	std::string versionRight = "latest";

	if (version != "latest") {
		size_t pos = version.find("/");
		if (pos == std::string::npos) {
			versionRight = ResolveVersion(version);
			if (versionRight == "") {
				std::cout << "Invalid version '" << version << "'" << std::endl;
				versionRight = "latest";
			}
		} else {
			branch = version.substr(0, pos);
			versionRight = version.substr(pos + 1);
			versionRight = ResolveVersion(versionRight);
			if (versionRight == "") {
				std::cout << "Invalid version '" << version << "'" << std::endl;
				versionRight = "latest";
			}
		}
	}
	return BranchVersionPair(branch, versionRight);
}

bool IsInteger(std::string number) {
	if (number.size() == 0) {
		return false;
	}
	for (char c : number) {
		if (c < '0' || c > '9') {
			return false;
		}
	}
	return true;
}

bool StartsWith(std::string text, std::string with) {
	if (text.size() < with.size()) {
		return false;
	}
	for (size_t i = 0; i < with.size(); i++) {
		if (text[i] != with[i]) {
			return false;
		}
	}

	return true;
}

void GetVersionsFromServer(const std::vector<VersionServer> &servers) {
	GetRuntimeConfig().versions.clear();
	for (const VersionServer &s : servers) {
		std::string content = GetContentHTTP(s.url);
		std::vector<Version> versions = ParseVersions(content);
		for (const Version &v : versions) {
			GetRuntimeConfig().versions[s.name].push_back(v);
		}
	}
}

void InstallSowa(std::filesystem::path appPath, std::string url, BranchVersionPair ver, bool installRunner) {
	std::cout << "Installing from " << url << std::endl;
	std::vector<unsigned char> file = GetFileHTTP(url);

	zip_source_t *src = zip_source_buffer_create(file.data(), file.size(), 1, nullptr);
	zip_source_keep(src);
	zip_t *za = zip_open_from_source(src, 0, nullptr);

	int files = zip_get_num_files(za);

	for (int i = 0; i < files; i++) {
		const char *name = zip_get_name(za, i, 0);
		if (name == nullptr) {
			fprintf(stderr, "Error getting name of file #%d: %s\n", i, zip_strerror(za));
			continue;
		}

		zip_stat_t stat;
		zip_stat_index(za, i, 0, &stat);
		if (stat.size == 0) {
			continue; // is directory
		}

		enum class FileType {
			none = 0,
			sowa,
			runner,
		};
		FileType type = FileType::none;
		std::filesystem::path fsName = name;
		if (StartsWith(fsName.filename().string(), "sowa-")) {
			type = FileType::sowa;
		} else if(ver.branch == "main") {
			if(installRunner && StartsWith(fsName.filename().string(), "sowa")) {
				type = FileType::runner;
			}
		}
		if(type == FileType::none)
			continue;

		struct zip_file *file = zip_fopen_index(za, i, 0);
		if (file == NULL) {
			fprintf(stderr, "Error opening file #%d: %s\n", i, zip_strerror(za));
			continue;
		}

		std::vector<char> buffer(stat.size);
		std::vector<char> file_data;
		while (true) {
			ssize_t num_bytes = zip_fread(file, buffer.data(), buffer.size());
			if (num_bytes < 0) {
				fprintf(stderr, "Error reading file #%d: %s\n", i, zip_file_strerror(file));
				zip_fclose(file);
				zip_close(za);
				continue;
			}
			if (num_bytes == 0) {
				break;
			}
			file_data.insert(file_data.end(), buffer.begin(), buffer.begin() + num_bytes);
		}
		zip_fclose(file);

		std::filesystem::path execPath;
		if(type == FileType::sowa) {
			execPath = (appPath / "bin" / ver.branch / fsName.filename());
		} else {
			execPath = appPath / fsName.filename();
		}
		std::cout << "Saving file to " << execPath.string() << std::endl;

		if(type == FileType::runner) {
			int res = std::rename(execPath.string().c_str(), (execPath.parent_path() / ("tmp." + execPath.filename().string())).c_str());
			if (res != 0) {
				std::cout << "Failed to save file:\n error: " << RED << strerror(errno) << RESET << std::endl;
			}
		}

		std::ofstream fileStream((execPath).string().c_str(), std::ios::out | std::ios::binary);
		if (fileStream.is_open()) {
			fileStream.write(buffer.data(), buffer.size());
			fileStream.close();
		} else {
			std::cout << "Failed to save file:\n error: " << RED << strerror(errno) << RESET << std::endl;
			continue;
		}

		std::cout << "\t" GREEN << execPath.filename().string() << RESET << " successfully installed!\n";
#ifdef SW_LINUX
		std::cout << "\t" << CYAN << "$ chmod +x " << execPath.string() << RESET << " to be able to run it.\n";
#endif
		std::cout << std::endl;
	}

	zip_close(za);
	zip_source_close(src);
}


VersionName::VersionName() {
	major = 0;
	minor = 0;
	patch = 0;
	tag = VersionTag::alpha;
	rev = 0;
}

VersionName::VersionName(std::string version) {
	major = 0;
	minor = 0;
	patch = 0;
	tag = VersionTag::alpha;
	rev = 0;

	BranchVersionPair ver = ResolveFullVersion(version);
	// 1.0.0-a2
	auto tokens = Split(ver.version, ".");
	if (tokens.size() != 3) {
		return;
	}
	try {
		auto rTokens = Split(tokens[2], "-");
		if (rTokens.size() != 2) {
			return;
		}

		major = std::stoi(tokens[0]);
		minor = std::stoi(tokens[1]);
		patch = std::stoi(rTokens[0]);
		if (StartsWith(rTokens[1], "a")) {
			tag = VersionTag::alpha;
			rev = std::stoi(rTokens[1].substr(strlen("a")));
		} else if (StartsWith(rTokens[1], "b")) {
			tag = VersionTag::beta;
			rev = std::stoi(rTokens[1].substr(strlen("b")));
		} else if (StartsWith(rTokens[1], "rc")) {
			tag = VersionTag::rc;
			rev = std::stoi(rTokens[1].substr(strlen("rc")));
		} else if (StartsWith(rTokens[1], "stable")) {
			tag = VersionTag::stable;
			rev = std::stoi(rTokens[1].substr(strlen("stable")));
		}

	} catch (const std::exception &e) {
	}
}

std::string VersionName::Repr() {
	std::string repr = std::to_string(major) + "." + std::to_string(minor) + "." + std::to_string(patch) + "-";
	if (tag == VersionTag::alpha)
		repr += "a";
	else if (tag == VersionTag::alpha)
		repr += "b";
	else if (tag == VersionTag::alpha)
		repr += "rc";
	else if (tag == VersionTag::alpha)
		repr += "stable";

	repr += std::to_string(rev);
	return repr;
}


void TempFileCleanup(std::filesystem::path appPath) {
	for(auto entry : std::filesystem::recursive_directory_iterator(appPath)) {
		if(StartsWith(entry.path().filename(), "tmp.")) {
			std::filesystem::remove_all(entry);
		}
	}
}