#ifndef SW_NEW_FILE_SERVER_HXX
#define SW_NEW_FILE_SERVER_HXX

#include <string>
#include <unordered_map>

#include "core/error.hxx"
#include "data/file/file_entry.hxx"
#include "data/file_buffer.hxx"

enum ReadWriteFlags : uint32_t {
	ReadWriteFlags_None = 0,
	ReadWriteFlags_AsText = 1 << 0,
	ReadWriteFlags_FullPath = 1 << 1,
};

using ReadWriteFlagBits = uint32_t;

class FileServer {
  public:
	static FileServer &get();

	Result<file_buffer *> load_file(const char *path, ReadWriteFlagBits flags = 0);
	ErrorCode save_file(const void *data, int size, const char *path, ReadWriteFlagBits flags = 0);

	std::vector<FileEntry> read_dir(const char *path, bool recursive = false);
	bool exists(const char *path);

	std::filesystem::path get_filepath(const std::string &path, ReadWriteFlagBits flags = 0);

  private:
	FileServer();
	~FileServer();

	std::unordered_map<std::string, file_buffer> m_buffers;
};

#endif // SW_NEW_FILE_SERVER_HXX