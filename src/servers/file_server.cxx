#include "file_server.hxx"

#include <fstream>

#include "core/app.hxx"

FileServer &FileServer::get() {
	static FileServer *server = new FileServer;
	return *server;
}

Result<file_buffer *> FileServer::load_file(const char *p, ReadWriteFlagBits flags) {
	std::filesystem::path path = get_filepath(p, flags);
	if (path == "")
		return ERR_FILE_NOT_FOUND;

	try {
		std::ifstream::openmode om = std::ifstream::in;

		if (flags & ReadWriteFlags_AsText) {
			om |= std::ifstream::binary;
		}

		std::ifstream ifstream(path, om);
		if (!ifstream.is_open())
			return ERR_FILE_NOT_FOUND;

		ifstream.unsetf(std::ios::skipws);

		std::streampos fileSize;
		ifstream.seekg(0, std::ios::end);
		fileSize = ifstream.tellg();
		ifstream.seekg(0, std::ios::beg);

		if (fileSize < 0)
			return ERR_FILE_NOT_FOUND;

		file_buffer &buf = m_buffers[path.string()];
		buf.m_buffer.clear();
		buf.m_buffer.resize(fileSize);
		ifstream.read((char *)buf.m_buffer.data(), fileSize);

		ifstream.close();
		return &buf;

	} catch (const std::exception &ex) {
		return ERR_FAILED;
	}

	return ERR_FAILED;
}

ErrorCode FileServer::save_file(const void *data, int size, const char *p, ReadWriteFlagBits flags) {
	std::filesystem::path path = get_filepath(p, flags);
	if (path == "")
		return ERR_FILE_NOT_FOUND;

	std::ofstream ofstream(path);
	if (!ofstream.is_open())
		return ERR_FILE_NOT_FOUND;

	ofstream.write(reinterpret_cast<const char *>(data), size);

	return OK;
}

std::vector<FileEntry> FileServer::read_dir(const char *p, bool recursive) {
	std::filesystem::path base = get_filepath("");

	std::vector<FileEntry> files;
	std::string path = get_filepath(p).string();
	if (path == "")
		return files;

	if (!std::filesystem::exists(path))
		return std::vector<FileEntry>{};

	if (recursive) {
		for (auto &entry : std::filesystem::recursive_directory_iterator(path)) {
			FileEntry file;
			file.m_isDirectory = entry.is_directory();
			file.m_path = std::filesystem::relative(entry.path(), base);

			files.push_back(file);
		}
	} else {
		for (auto &entry : std::filesystem::directory_iterator(path)) {
			FileEntry file;
			file.m_isDirectory = entry.is_directory();
			file.m_path = std::filesystem::relative(entry.path(), base);

			files.push_back(file);
		}
	}

	return files;
}

bool FileServer::exists(const char *path) {
	return std::filesystem::exists(get_filepath(path));
}

std::filesystem::path FileServer::get_filepath(const std::string &p, ReadWriteFlagBits flags) {
	if (flags & ReadWriteFlags_FullPath)
		return p;

	// path: /images/image.png
	std::string_view path = p;
	if (p.size() > 0 && p[0] == '/')
		path = std::string_view(p.c_str() + 1, p.size() - 1);

	return App::get().m_appPath / path;
}

FileServer::FileServer() {
}

FileServer::~FileServer() {
}