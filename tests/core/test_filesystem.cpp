#include <doctest/doctest.h>

#include <cstring>
#include <filesystem>
#include <fstream>

#include "core/filesystem/file_server.hpp"
#include "core/filesystem/file_system.hpp"

using namespace sowa;

namespace {

std::filesystem::path makeTempDir() {
	const auto dir =
		std::filesystem::temp_directory_path() / "sowa_fs_test";
	std::filesystem::remove_all(dir);
	std::filesystem::create_directories(dir);
	return dir;
}

void writeFile(const std::filesystem::path &path, std::string_view contents) {
	std::filesystem::create_directories(path.parent_path());
	std::ofstream out(path, std::ios::binary);
	out.write(contents.data(), static_cast<std::streamsize>(contents.size()));
}

} // namespace

TEST_CASE("FileSystem::Resolve parses scheme paths and rejects escapes") {
	FileSystem fs;

	auto ok = fs.Resolve("res://sprites/player.png");
	CHECK(ok);
	CHECK(ok.value.scheme == "res");
	CHECK(ok.value.path == "sprites/player.png");

	auto rooted = fs.Resolve("res:///foo");
	CHECK(rooted);
	CHECK(rooted.value.path == "foo");

	CHECK(fs.Resolve("noscheme").error == FsError::InvalidPath);
	CHECK(fs.Resolve("res://../secret").error == FsError::InvalidPath);
	CHECK(fs.Resolve("res://a/../../b").error == FsError::InvalidPath);
}

TEST_CASE("FileSystem MountFolder Load/Save/ReadDirectory") {
	const auto root = makeTempDir();
	writeFile(root / "hello.txt", "hello");
	std::filesystem::create_directories(root / "sub");
	writeFile(root / "sub" / "nested.txt", "nested");

	FileSystem fs;
	fs.MountFolder("res", root);
	CHECK(fs.Has("res"));

	auto loaded = fs.Load("res://hello.txt");
	REQUIRE(loaded);
	REQUIRE(loaded.value);
	CHECK(loaded.value->Size() == 5);
	CHECK(std::string(reinterpret_cast<const char *>(loaded.value->Data()),
					  loaded.value->Size()) == "hello");

	CHECK(fs.Load("res://missing.txt").error == FsError::NotFound);
	CHECK(fs.Load("res://sub").error == FsError::NotAFile);

	auto listing = fs.ReadDirectory("res://");
	REQUIRE(listing);
	REQUIRE(listing.value.size() == 2);
	CHECK(listing.value[0].name == "hello.txt");
	CHECK_FALSE(listing.value[0].isDirectory);
	CHECK(listing.value[1].name == "sub");
	CHECK(listing.value[1].isDirectory);

	auto subListing = fs.ReadDirectory("res://sub");
	REQUIRE(subListing);
	REQUIRE(subListing.value.size() == 1);
	CHECK(subListing.value[0].name == "nested.txt");

	FileData out;
	const std::string payload = "saved";
	out.bytes.resize(payload.size());
	std::memcpy(out.bytes.data(), payload.data(), payload.size());
	CHECK(fs.Save("res://hello.txt", out) == FsError::Ok);

	auto reloaded = fs.Load("res://hello.txt");
	REQUIRE(reloaded);
	CHECK(std::string(reinterpret_cast<const char *>(reloaded.value->Data()),
					  reloaded.value->Size()) == "saved");

	CHECK(fs.Save("res://sub", out) == FsError::NotAFile);
	CHECK(fs.Load("data://x").error == FsError::InvalidPath);

	std::filesystem::remove_all(root);
}

TEST_CASE("FileServer Save defaults to NotSupported") {
	class ReadOnlyServer : public FileServer {
	  public:
		FsResult<Ref<FileData>> Load(const std::filesystem::path &) override {
			return {nullptr, FsError::NotFound};
		}
	};

	FileSystem fs;
	fs.Register("ro", MakeScope<ReadOnlyServer>());
	FileData data;
	CHECK(fs.Save("ro://file.txt", data) == FsError::NotSupported);
	CHECK(fs.ReadDirectory("ro://").error == FsError::NotSupported);
	CHECK(fs.CreateFile("ro://file.txt") == FsError::NotSupported);
	CHECK(fs.CreateDirectory("ro://folder") == FsError::NotSupported);
	CHECK(fs.Move("ro://old", "ro://new") == FsError::NotSupported);
	CHECK(fs.RemoveAll("ro://file.txt") == FsError::NotSupported);
}

TEST_CASE("FileSystem creates, renames, and recursively removes assets") {
	const auto root = makeTempDir();
	FileSystem fs;
	fs.MountFolder("res", root);

	CHECK(fs.CreateDirectory("res://levels") == FsError::Ok);
	CHECK(fs.CreateDirectory("res://levels") == FsError::AlreadyExists);
	CHECK(fs.CreateFile("res://levels/one.scene") == FsError::Ok);
	CHECK(fs.CreateFile("res://levels/one.scene") == FsError::AlreadyExists);
	CHECK(std::filesystem::is_regular_file(root / "levels" / "one.scene"));

	CHECK(fs.Move("res://levels/one.scene", "res://levels/two.scene") ==
		  FsError::Ok);
	CHECK_FALSE(std::filesystem::exists(root / "levels" / "one.scene"));
	CHECK(std::filesystem::exists(root / "levels" / "two.scene"));
	CHECK(fs.Move("res://levels/missing.scene", "res://levels/three.scene") ==
		  FsError::NotFound);

	std::filesystem::create_directories(root / "levels" / "nested");
	writeFile(root / "levels" / "nested" / "data.txt", "data");
	CHECK(fs.RemoveAll("res://levels") == FsError::Ok);
	CHECK_FALSE(std::filesystem::exists(root / "levels"));
	CHECK(fs.RemoveAll("res://levels") == FsError::NotFound);

	// A virtual mount is never a valid destructive target.
	CHECK(fs.RemoveAll("res://") == FsError::InvalidPath);
	CHECK(std::filesystem::exists(root));
	CHECK(fs.Move("res://", "res://renamed") == FsError::InvalidPath);

	std::filesystem::remove_all(root);
}

TEST_CASE("FileSystem refuses moves between mounted servers") {
	const auto root = makeTempDir();
	const auto other = root / "other";
	std::filesystem::create_directories(other);
	writeFile(root / "asset.txt", "asset");

	FileSystem fs;
	fs.MountFolder("res", root);
	fs.MountFolder("tmp", other);
	CHECK(fs.Move("res://asset.txt", "tmp://asset.txt") ==
		  FsError::NotSupported);
	CHECK(std::filesystem::exists(root / "asset.txt"));

	std::filesystem::remove_all(root);
}

TEST_CASE("folder mounts reject symlinks that leave their root") {
	const auto root = makeTempDir();
	const auto outside = root.parent_path() / "sowa_fs_outside_review";
	std::filesystem::create_directories(outside);
	writeFile(outside / "keep.txt", "keep");
	std::error_code ec;
	std::filesystem::create_directory_symlink(outside, root / "escape", ec);
	REQUIRE_FALSE(ec);
	FileSystem fs;
	fs.MountFolder("res", root);
	CHECK(fs.Load("res://escape/keep.txt").error == FsError::InvalidPath);
	CHECK(fs.ReadDirectory("res://escape").error == FsError::InvalidPath);
	CHECK(fs.Save("res://escape/keep.txt", {}) == FsError::InvalidPath);
	CHECK(fs.CreateFile("res://escape/new.txt") == FsError::InvalidPath);
	CHECK(fs.CreateDirectory("res://escape/new") == FsError::InvalidPath);
	CHECK(fs.Move("res://escape/keep.txt", "res://lost.txt") == FsError::InvalidPath);
	CHECK(fs.RemoveAll("res://escape/keep.txt") == FsError::InvalidPath);
	CHECK(std::filesystem::file_size(outside / "keep.txt") == 4);
	std::filesystem::remove_all(root);
	std::filesystem::remove_all(outside);
}
