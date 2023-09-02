#ifndef SW_FILE_ENTRY_HXX
#define SW_FILE_ENTRY_HXX
#pragma once

#include <filesystem>

class FileEntry {
  public:
	FileEntry() = default;

	const std::filesystem::path &Path() const { return m_path; }
	bool IsDirectory() const { return m_isDirectory; }
	bool IsFile() const { return !m_isDirectory; }

  private:
	friend class FileServer;

	bool m_isDirectory = false;
	std::filesystem::path m_path;
};

#endif // SW_FILE_ENTRY_HXX