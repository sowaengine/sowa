#ifndef SW_FILE_SERVER_HXX
#define SW_FILE_SERVER_HXX
#pragma once

#include <string>

#include "core/app.hxx"
#include "data/file_buffer.hxx"

class FileServer {
  public:
	FileServer();

	static void Create(App *app);
	static FileServer &GetInstance();

	Error ReadFileString(const char *path, std::string &buffer);
	Error WriteFileString(const char *path, const std::string &buffer);

  private:
	std::string getFilepath(const std::string &path);

	App *m_pApp = nullptr;
};

#endif // SW_FILE_SERVER_HXX