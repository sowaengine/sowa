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

FileServer &FileServer::GetInstance() {
	return s_instance;
}

Error FileServer::ReadFileString(const char *p, std::stringstream &stream) {
	std::string path = getFilepath(p);
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

Error FileServer::ReadFileString(const char *p, std::string &buffer) {
	buffer = "";

	std::stringstream s;
	Error err = ReadFileString(p, s);
	buffer = s.str();
	return OK;
}

Error FileServer::WriteFileString(const char *p, const std::string &buffer) {
	std::string path = getFilepath(p);
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

Error FileServer::ReadFileBytes(const char *p, file_buffer &buffer) {
	std::string path = getFilepath(p);
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

std::string FileServer::getFilepath(const std::string &path) {
	// scheme://path/to/file
	auto tokens = Split(path, "://");
	if (tokens.size() < 2) {
		return "";
	}

	const std::string &scheme = tokens[0];
	if (scheme == "res") {
		return (m_pApp->m_appPath / std::filesystem::path(tokens[1])).string();
	}

	return "";
}