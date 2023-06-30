#include "file_server.hxx"

#include "utils/utils.hxx"

#include <fstream>
#include <iostream>

static FileServer s_instance;

FileServer::FileServer() {
}

void FileServer::Create(App *app) {
	s_instance.m_pApp = app;
}

FileServer &FileServer::GetInstance() {
	return s_instance;
}

Error FileServer::ReadFileString(const char *p, std::string &buffer) {
	buffer = "";

	std::string path = getFilepath(p);
	if (path == "") {
		return ERR_FILE_NOT_FOUND;
	}

	std::ifstream ifstream(path);
	if (!ifstream.is_open()) {
		return ERR_FILE_NOT_FOUND;
	}
	std::stringstream s;
	s << ifstream.rdbuf();

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

std::string FileServer::getFilepath(const std::string &path) {
	// scheme://path/to/file
	auto tokens = Split(path, "://");
	if (tokens.size() < 2) {
		return "";
	}

	const std::string &scheme = tokens[0];
	if (scheme == "res") {
		return m_pApp->m_appPath / tokens[1];
	}

	return "";
}