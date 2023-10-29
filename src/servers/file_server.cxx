#include "file_server.hxx"

#include "utils/utils.hxx"

#include <fstream>
#include <iostream>
#include <sstream>

static FileServer s_instance;

FileServer::FileServer() {
}

void FileServer::Create(App *app) {
	s_instance.m_pApp = app;
}

FileServer &FileServer::get() {
	return s_instance;
}

ErrorCode FileServer::ReadFileString(const char *p, std::stringstream &stream) {
	std::filesystem::path path = getFilepath(p);
	if (path == "") {
		return ERR_FILE_NOT_FOUND;
	}

	std::ifstream ifstream(path);
	if (!ifstream.is_open()) {
		return ERR_FILE_NOT_FOUND;
	}

	stream << ifstream.rdbuf();
	return OK;
}

ErrorCode FileServer::ReadFileString(const char *p, std::string &buffer) {
	buffer = "";

	std::stringstream s;
	ErrorCode err = ReadFileString(p, s);
	buffer = s.str();
	return OK;
}

ErrorCode FileServer::WriteFileString(const char *p, const std::string &buffer) {
	std::filesystem::path path = getFilepath(p);
	if (path == "") {
		return ERR_FILE_NOT_FOUND;
	}

	std::ofstream ofstream(path);
	if (!ofstream.is_open()) {
		return ERR_FILE_NOT_FOUND;
	}
	ofstream << buffer << std::endl;

	return OK;
}

ErrorCode FileServer::WriteFileBytes(const char *p, file_buffer &buffer) {
	std::filesystem::path path = getFilepath(p);
	if (path == "") {
		return ERR_FILE_NOT_FOUND;
	}

	std::ofstream ofstream(path);
	if (!ofstream.is_open()) {
		return ERR_FILE_NOT_FOUND;
	}
	ofstream.write(reinterpret_cast<const char *>(buffer.data()), buffer.size());

	return OK;
}

ErrorCode FileServer::ReadFileBytes(const char *p, file_buffer &buffer) {

	std::filesystem::path path = getFilepath(p);
	if (path == "") {
		return ERR_FILE_NOT_FOUND;
	}

	try {
		std::ifstream ifstream(path, std::ifstream::binary);
		if (!ifstream.is_open()) {
			return ERR_FILE_NOT_FOUND;
		}

		ifstream.unsetf(std::ios::skipws);

		std::streampos fileSize;
		ifstream.seekg(0, std::ios::end);
		fileSize = ifstream.tellg();
		ifstream.seekg(0, std::ios::beg);

		if (fileSize < 0) {
			return ERR_FILE_NOT_FOUND;
		}

		buffer.m_buffer.clear();
		buffer.m_buffer.resize(fileSize);
		ifstream.read((char *)buffer.m_buffer.data(), fileSize);

		ifstream.close();
	} catch (const std::exception &ex) {
		return ERR_FAILED;
	}

	return OK;
}

std::vector<FileEntry> FileServer::ReadDir(const char *p, bool recursive) {
	std::filesystem::path base = getFilepath("res://");

	std::vector<FileEntry> files;
	std::string path = getFilepath(p);
	if (path == "")
		return files;

	if (recursive) {
		for (auto &entry : std::filesystem::recursive_directory_iterator(path)) {
			FileEntry file;
			file.m_isDirectory = entry.is_directory();
			file.m_path = std::filesystem::path("res://") / (std::filesystem::relative(entry.path(), base));

			files.push_back(file);
		}
	} else {
		for (auto &entry : std::filesystem::directory_iterator(path)) {
			FileEntry file;
			file.m_isDirectory = entry.is_directory();
			file.m_path = std::filesystem::path("res://") / (std::filesystem::relative(entry.path(), base));

			files.push_back(file);
		}
	}

	return files;
}

std::filesystem::path FileServer::getFilepath(const std::string &path) {
	// scheme://path/to/file
	auto tokens = Utils::Split(path, "://");
	if (tokens.size() == 1) {
		tokens.push_back("");
	}
	if (tokens.size() < 2) {
		return "";
	}

	const std::string &scheme = tokens[0];
	if (scheme == "res") {
		std::filesystem::path p = m_pApp->m_appPath / std::filesystem::path(tokens[1]);
		std::string r = std::filesystem::relative(p, m_pApp->m_appPath).string();
		if (r.length() >= 3 && (r[0] == '.' && r[1] == '.' && (r[2] == '/' || r[2] == '\\'))) {
			return "";
		}

		return p;
	} else if (scheme == "abs") {
		return tokens[1];
	}

	return "";
}