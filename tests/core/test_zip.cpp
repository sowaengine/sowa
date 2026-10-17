#include <doctest/doctest.h>

#include <cstring>
#include <filesystem>
#include <fstream>

#include "core/filesystem/file_system.hpp"
#include "core/filesystem/zip_builder.hpp"
#include "core/filesystem/zip_file_server.hpp"
#include "core/project_settings.hpp"

using namespace sowa;

namespace {
struct ZipTemp {
	std::filesystem::path root = std::filesystem::temp_directory_path() / "sowa_zip_test";

	ZipTemp() {
		std::filesystem::remove_all(root);
		std::filesystem::create_directories(root);
	}

	~ZipTemp() {
		std::filesystem::remove_all(root);
	}
};

FileData bytes(std::string_view text) {
	FileData result;
	result.bytes.resize(text.size());
	if (!text.empty()) {
		std::memcpy(result.Data(), text.data(), text.size());
	}
	return result;
}

std::string contents(const FsResult<Ref<FileData>> &result) {
	REQUIRE(result);
	if (!result.value->Size()) {
		return {};
	}
	return {reinterpret_cast<const char *>(result.value->Data()), result.value->Size()};
}

void write(const std::filesystem::path &path, std::string_view data) {
	std::ofstream out(path, std::ios::binary);
	out.write(data.data(), static_cast<std::streamsize>(data.size()));
}

std::filesystem::path fixture(std::string_view name) {
	return std::filesystem::path(SOWA_TEST_FIXTURES_DIR) / "zip" / name;
}
} // namespace

TEST_CASE("ZIP reads independent Python stored, deflated, UTF-8 and ZIP64 entries") {
	FileSystem fs;
	REQUIRE(fs.MountZip("res", fixture("python.zip")) == FsError::Ok);
	auto stored = fs.Load("res://stored.bin");
	REQUIRE(stored);
	REQUIRE(stored.value->Size() == 256);
	for (unsigned i = 0; i < 256; ++i) {
		CHECK(stored.value->bytes[i] == std::byte(i));
	}
	std::string expected;
	for (int i = 0; i < 100; ++i) {
		expected += "compressed asset\n";
	}
	CHECK(contents(fs.Load("res://nested/./deflated.txt")) == expected);
	CHECK(contents(fs.Load("res://zip64.txt")) == "zip64 local header");
	CHECK(contents(fs.Load("res://textures/oyuncu-ç.txt")) == "utf8");
	CHECK(contents(fs.Load("res://long/" + std::string(250, 'a') + '/' + std::string(250, 'b') +
						   "/asset.txt")) == "long name");
	CHECK(contents(fs.Load("res://empty.txt")).empty());
	CHECK(fs.Load("res://Stored.bin").error == FsError::NotFound);
	CHECK(fs.Load("res://nested").error == FsError::NotAFile);
	CHECK(fs.Load("res://").error == FsError::NotAFile);
	CHECK(fs.ReadDirectory("res://stored.bin").error == FsError::NotADirectory);
	CHECK(fs.ReadDirectory("res://missing").error == FsError::NotFound);
	auto empty = fs.ReadDirectory("res://empty/");
	REQUIRE(empty);
	CHECK(empty.value.empty());
	auto listing = fs.ReadDirectory("res://nested/");
	REQUIRE(listing);
	REQUIRE(listing.value.size() == 1);
	CHECK(listing.value[0].name == "deflated.txt");
	CHECK_FALSE(listing.value[0].isDirectory);
	auto root = fs.ReadDirectory("res://");
	REQUIRE(root);
	REQUIRE(root.value.size() == 7);
	CHECK(root.value[0].name == "empty");
	CHECK(root.value[0].isDirectory);
	CHECK(root.value[3].name == "nested");
	CHECK(root.value[3].isDirectory);
}

TEST_CASE("ZIP mount is read-only and failed replacement preserves current server") {
	ZipTemp temp;
	FileSystem fs;
	REQUIRE(fs.MountZip("res", fixture("python.zip")) == FsError::Ok);
	auto *server = fs.Get("res");
	CHECK(fs.Save("res://stored.bin", bytes("overwrite")) == FsError::NotSupported);
	CHECK(fs.CreateFile("res://new") == FsError::NotSupported);
	CHECK(fs.CreateDirectory("res://new") == FsError::NotSupported);
	CHECK(fs.Move("res://stored.bin", "res://other") == FsError::NotSupported);
	CHECK(fs.RemoveAll("res://") == FsError::NotSupported);
	CHECK(fs.RemoveAll("res://stored.bin") == FsError::NotSupported);
	CHECK(fs.MountZip("res", temp.root / "missing.zip") == FsError::NotFound);
	CHECK(fs.Get("res") == server);
	CHECK(contents(fs.Load("res://zip64.txt")) == "zip64 local header");
	write(temp.root / "invalid.zip", "not a zip");
	CHECK(fs.MountZip("bad", temp.root / "invalid.zip") == FsError::IoError);
	CHECK_FALSE(fs.Has("bad"));
	CHECK(fs.MountZip("bad", temp.root) == FsError::NotAFile);
	ZipFileServer bad(temp.root / "missing.zip");
	CHECK(bad.Load("x").error == FsError::NotFound);
	CHECK(bad.ReadDirectory("").error == FsError::NotFound);
}

TEST_CASE("ZIP rejects unsafe names and ambiguous file trees") {
	for (const auto *name : {"traversal.zip", "absolute.zip", "backslash.zip"}) {
		INFO(name);
		CHECK(ZipFileServer(fixture(name)).Error() == FsError::InvalidPath);
	}
	for (const auto *name : {"duplicate.zip", "collision.zip"}) {
		INFO(name);
		CHECK(ZipFileServer(fixture(name)).Error() == FsError::AlreadyExists);
	}
	ZipFileServer server(fixture("python.zip"));
	for (const auto *path : {"../stored.bin",
			 "/stored.bin",
			 "nested/../stored.bin",
			 "C:/stored.bin",
			 "nested\\deflated.txt"}) {
		INFO(path);
		CHECK(server.Load(path).error == FsError::InvalidPath);
		CHECK(server.ReadDirectory(path).error == FsError::InvalidPath);
	}
}

TEST_CASE("ZIP verifies payload CRC on load") {
	ZipFileServer server(fixture("corrupt.zip"));
	REQUIRE(server.Error() == FsError::Ok);
	CHECK(server.Load("stored.bin").error == FsError::IoError);
	CHECK(contents(server.Load("zip64.txt")) == "zip64 local header");
}

TEST_CASE("ZipBuilder packages memory, files and recursive directories for res") {
	ZipTemp temp;
	const auto source = temp.root / "source";
	std::filesystem::create_directories(source / "scenes");
	std::filesystem::create_directories(source / "empty");
	write(source / "project.sowa", "Application:\n  Name: Zip Game\n");
	write(source / "scenes" / "main.scene", "scene bytes");
	write(temp.root / "external", "outside");
	FileData data = bytes(std::string("binary\0data", 11));
	ZipBuilder builder;
	builder.CompressionLevel(9)
		.AddDirectory(source)
		.AddFile("extra/data.bin", temp.root / "external")
		.AddData("memory.bin", data)
		.AddData("zero", {});
	data.bytes.clear();
	REQUIRE(builder.Error() == FsError::Ok);
	REQUIRE(builder.Build(temp.root / "Game.zip") == FsError::Ok);
	FileSystem fs;
	REQUIRE(fs.MountZip("res", temp.root / "Game.zip") == FsError::Ok);
	ProjectSettings settings;
	REQUIRE(settings.Load(fs));
	CHECK(settings.application.name == "Zip Game");
	CHECK(contents(fs.Load("res://scenes/main.scene")) == "scene bytes");
	CHECK(contents(fs.Load("res://extra/data.bin")) == "outside");
	CHECK(contents(fs.Load("res://memory.bin")) == std::string("binary\0data", 11));
	CHECK(contents(fs.Load("res://zero")).empty());
	auto empty = fs.ReadDirectory("res://empty");
	REQUIRE(empty);
	CHECK(empty.value.empty());
	CHECK(builder.Build(temp.root / "Game.zip") == FsError::AlreadyExists);
	CHECK(contents(fs.Load("res://extra/data.bin")) == "outside");
	REQUIRE(ZipBuilder()
				.CompressionLevel(0)
				.AddDirectory(source, "assets")
				.Build(temp.root / "stored.zip") == FsError::Ok);
	ZipFileServer prefixed(temp.root / "stored.zip");
	CHECK(contents(prefixed.Load("assets/scenes/main.scene")) == "scene bytes");
	CHECK(prefixed.Load("scenes/main.scene").error == FsError::NotFound);
}

TEST_CASE("ZipBuilder errors are sticky and do not publish a partial archive") {
	ZipTemp temp;
	for (const auto *path : {"", "/absolute", "../escape", "a/../b", "a\\b", "C:/b", "dir/"}) {
		INFO(path);
		ZipBuilder builder;
		builder.AddData(path, {}).AddData("valid", {});
		CHECK(builder.Error() == FsError::InvalidPath);
		CHECK(builder.Build(temp.root / "bad.zip") == FsError::InvalidPath);
		CHECK_FALSE(std::filesystem::exists(temp.root / "bad.zip"));
	}
	CHECK(ZipBuilder().AddData("same", {}).AddData("./same", {}).Error() == FsError::AlreadyExists);
	CHECK(ZipBuilder().AddData("a/b", {}).AddData("a", {}).Error() == FsError::AlreadyExists);
	CHECK(ZipBuilder().AddData("a", {}).AddData("a/b", {}).Error() == FsError::AlreadyExists);
	CHECK(ZipBuilder().CompressionLevel(10).Error() == FsError::NotSupported);
	CHECK(ZipBuilder().AddFile("missing", temp.root / "missing").Error() == FsError::NotFound);
	CHECK(ZipBuilder().AddDirectory(temp.root / "missing").Error() == FsError::NotFound);
	CHECK(ZipBuilder().AddFile("directory", temp.root).Error() == FsError::NotAFile);
	write(temp.root / "file", "bytes");
	CHECK(ZipBuilder().AddDirectory(temp.root / "file").Error() == FsError::NotADirectory);
	ZipBuilder delayed;
	delayed.AddData("first", bytes("ok")).AddFile("gone", temp.root / "file");
	std::filesystem::remove(temp.root / "file");
	CHECK(delayed.Build(temp.root / "partial.zip") == FsError::IoError);
	CHECK_FALSE(std::filesystem::exists(temp.root / "partial.zip"));
	CHECK(ZipBuilder().Build(temp.root / "missing" / "out.zip") == FsError::IoError);
	REQUIRE(ZipBuilder().Build(temp.root / "empty.zip") == FsError::Ok);
	ZipFileServer empty(temp.root / "empty.zip");
	REQUIRE(empty.Error() == FsError::Ok);
	CHECK(empty.ReadDirectory("").value.empty());
}

TEST_CASE("ZIP reports unsupported compression without blocking directory listing") {
	ZipFileServer server(fixture("unsupported.zip"));
	REQUIRE(server.Error() == FsError::Ok);
	auto listing = server.ReadDirectory("");
	REQUIRE(listing);
	REQUIRE(listing.value.size() == 1);
	CHECK(listing.value[0].name == "bzip2.txt");
	CHECK(server.Load("bzip2.txt").error == FsError::NotSupported);
}

TEST_CASE("ZipBuilder rejects symlink inputs and preserves existing output") {
	ZipTemp temp;
	write(temp.root / "original", "keep me");
	CHECK(ZipBuilder().AddData("new", {}).Build(temp.root / "original") == FsError::AlreadyExists);
	std::ifstream input(temp.root / "original");
	std::string text((std::istreambuf_iterator<char>(input)), {});
	CHECK(text == "keep me");
	std::error_code ec;
	std::filesystem::create_symlink(temp.root / "original", temp.root / "link", ec);
	if (!ec) {
		CHECK(ZipBuilder().AddFile("link", temp.root / "link").Error() == FsError::NotAFile);
		CHECK(ZipBuilder().AddDirectory(temp.root).Error() == FsError::NotAFile);
		CHECK(ZipBuilder().Build(temp.root / "link") == FsError::AlreadyExists);
		std::filesystem::create_symlink(temp.root / "absent", temp.root / "dangling", ec);
		REQUIRE_FALSE(ec);
		CHECK(ZipBuilder().Build(temp.root / "dangling") == FsError::AlreadyExists);
		CHECK_FALSE(std::filesystem::exists(temp.root / "absent"));
	}
}
