#ifndef SW_FILE_SERVER_HXX
#define SW_FILE_SERVER_HXX
#pragma once

#include <sstream>
#include <string>
#include <vector>

#include "core/app.hxx"
#include "data/file/file_entry.hxx"
#include "data/file_buffer.hxx"

class FileServer {
  public:
	FileServer();

	static void Create(App *app);
	static FileServer &get();

	ErrorCode ReadFileString(const char *path, std::stringstream &stream);
	ErrorCode ReadFileString(const char *path, std::string &buffer);
	ErrorCode WriteFileString(const char *path, const std::string &buffer);

	ErrorCode WriteFileBytes(const char *path, file_buffer &buffer);
	ErrorCode ReadFileBytes(const char *path, file_buffer &buffer);

	std::vector<FileEntry> ReadDir(const char *path, bool recursive = false);

	bool Exists(const char *path);

  private:
	std::filesystem::path getFilepath(const std::string &path);

	App *m_pApp = nullptr;
};

#endif // SW_FILE_SERVER_HXX