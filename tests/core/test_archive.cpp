#include <doctest/doctest.h>

#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>
#include <vector>

#include "core/reflection/enum_constant.hpp"
#include "core/filesystem/file_system.hpp"
#include "core/serialize/archive.hpp"

using namespace sowa;

namespace {

struct Settings : public Serializable {
	bool enabled = false;
	i32 count = 0;
	u32 mask = 0;
	f32 speed = 0.f;
	f64 precise = 0.0;
	std::string title;
	Vector2 position;
	Vector3 axis;
	Color tint;

	void Serialize(Archive &archive) override {
		archive.Field("enabled", enabled);
		archive.Field("count", count);
		archive.Field("mask", mask);
		archive.Field("speed", speed);
		archive.Field("precise", precise);
		archive.Field("title", title);
		archive.Field("position", position);
		archive.Field("axis", axis);
		archive.Field("tint", tint);
	}
};

enum class Mode {
	Relative = 0,
	Absolute = 7,
};

const std::vector<EnumConstant> &modeConstants() {
	static const std::vector<EnumConstant> constants{
		EnumConstant{"Relative", 0},
		EnumConstant{"Absolute", 7},
	};
	return constants;
}

std::filesystem::path makeTempDir(std::string_view name) {
	const auto dir = std::filesystem::temp_directory_path() / name;
	std::filesystem::remove_all(dir);
	std::filesystem::create_directories(dir);
	return dir;
}

std::string readFile(const std::filesystem::path &path) {
	std::ifstream in(path, std::ios::binary);
	return std::string(std::istreambuf_iterator<char>(in),
					   std::istreambuf_iterator<char>());
}

} // namespace

TEST_CASE("Archive round-trips every field type through one Serialize body") {
	Settings written;
	written.enabled = true;
	written.count = -42;
	written.mask = 4000000000u;
	written.speed = 12.5f;
	written.precise = 0.1234567890123;
	written.title = "Hello: world";
	written.position = Vector2(100.f, -50.f);
	written.axis = Vector3(0.f, 1.f, 0.f);
	written.tint = Color(0.25f, 0.5f, 0.75f, 0.5f);

	Archive saving = Archive::Save();
	written.Serialize(saving);
	const std::string text = saving.ToString();
	REQUIRE_FALSE(saving.HasErrors());

	Settings read;
	Archive loading = Archive::Load(text);
	REQUIRE(loading.IsValid());
	read.Serialize(loading);
	CHECK_FALSE(loading.HasErrors());

	CHECK(read.enabled == true);
	CHECK(read.count == -42);
	CHECK(read.mask == 4000000000u);
	CHECK(read.speed == doctest::Approx(12.5f));
	CHECK(read.precise == doctest::Approx(0.1234567890123));
	CHECK(read.title == "Hello: world");
	CHECK(read.position.x == doctest::Approx(100.f));
	CHECK(read.position.y == doctest::Approx(-50.f));
	CHECK(read.axis.y == doctest::Approx(1.f));
	CHECK(read.tint.b == doctest::Approx(0.75f));
	CHECK(read.tint.a == doctest::Approx(0.5f));
}

// A round-trip alone would pass even if the encoding were privately weird, as
// long as it were self-consistent. These two tests pin the actual document:
// what we emit, and that we can read YAML we did not write.
TEST_CASE("Archive writes vectors and colors as flow sequences") {
	Settings settings;
	settings.position = Vector2(100.f, 50.f);
	settings.tint = Color(1.f, 0.f, 0.f, 1.f);

	Archive saving = Archive::Save();
	settings.Serialize(saving);
	const std::string text = saving.ToString();

	CHECK(text.find("position: [100, 50]") != std::string::npos);
	CHECK(text.find("tint: [1, 0, 0, 1]") != std::string::npos);
}

TEST_CASE("Archive loads hand-written YAML") {
	Settings settings;
	Archive loading = Archive::Load(R"(
enabled: true
count: 5
title: from hand
position: [3, 4]
tint: [1, 0.5, 0]
)");
	REQUIRE(loading.IsValid());
	settings.Serialize(loading);

	CHECK_FALSE(loading.HasErrors());
	CHECK(settings.enabled == true);
	CHECK(settings.count == 5);
	CHECK(settings.title == "from hand");
	CHECK(settings.position.x == doctest::Approx(3.f));
	CHECK(settings.position.y == doctest::Approx(4.f));
	// Three components means opaque, regardless of what the target held.
	CHECK(settings.tint.g == doctest::Approx(0.5f));
	CHECK(settings.tint.a == doctest::Approx(1.f));
}

TEST_CASE("Absent keys leave values untouched and are not errors") {
	Settings settings;
	settings.count = 99;
	settings.title = "unchanged";

	Archive loading = Archive::Load("enabled: true");
	REQUIRE(loading.IsValid());
	settings.Serialize(loading);

	CHECK(settings.enabled == true);
	CHECK(settings.count == 99);
	CHECK(settings.title == "unchanged");
	CHECK_FALSE(loading.HasErrors());
}

TEST_CASE("Field reports whether a value was actually transferred") {
	i32 value = 3;
	Archive loading = Archive::Load("present: 7");
	CHECK(loading.Field("present", value));
	CHECK(value == 7);
	CHECK_FALSE(loading.Field("missing", value));
	CHECK(value == 7);
}

TEST_CASE("Malformed values are reported and leave the target alone") {
	Settings settings;
	settings.count = 99;
	settings.speed = 1.5f;
	settings.position = Vector2(8.f, 9.f);

	Archive loading = Archive::Load(R"(
count: not-a-number
speed: [1, 2]
position: 5
)");
	REQUIRE(loading.IsValid());
	settings.Serialize(loading);

	CHECK(settings.count == 99);
	CHECK(settings.speed == doctest::Approx(1.5f));
	CHECK(settings.position.x == doctest::Approx(8.f));

	REQUIRE(loading.Errors().size() == 3);
	CHECK(loading.Errors()[0].find("count") != std::string::npos);
	CHECK(loading.Errors()[0].find("not-a-number") != std::string::npos);
	CHECK(loading.Errors()[1].find("speed") != std::string::npos);
	CHECK(loading.Errors()[2].find("position") != std::string::npos);
}

TEST_CASE("A half-decoded sequence does not partially overwrite its target") {
	Vector3 axis(1.f, 2.f, 3.f);
	Archive loading = Archive::Load("axis: [9, oops, 9]");
	CHECK_FALSE(loading.Field("axis", axis));

	CHECK(axis.x == doctest::Approx(1.f));
	CHECK(axis.y == doctest::Approx(2.f));
	CHECK(axis.z == doctest::Approx(3.f));
}

TEST_CASE("Child nests both ways and prefixes error paths") {
	Archive saving = Archive::Save();
	{
		Archive outer = saving.Child("Rendering");
		Archive inner = outer.Child("Window");
		i32 width = 800;
		inner.Field("Width", width);
	}
	const std::string text = saving.ToString();
	CHECK(text.find("Rendering:") != std::string::npos);
	CHECK(text.find("Width: 800") != std::string::npos);

	Archive loading = Archive::Load(text);
	i32 width = 0;
	loading.Child("Rendering").Child("Window").Field("Width", width);
	CHECK(width == 800);

	Archive broken = Archive::Load("Rendering:\n  Window:\n    Width: wide\n");
	i32 unread = 1280;
	broken.Child("Rendering").Child("Window").Field("Width", unread);
	CHECK(unread == 1280);
	REQUIRE(broken.Errors().size() == 1);
	CHECK(broken.Errors()[0].rfind("Rendering/Window/Width:", 0) == 0);
}

TEST_CASE("A missing child is inert rather than an error") {
	Archive loading = Archive::Load("Other: 1");
	Archive missing = loading.Child("Rendering").Child("Window");

	i32 width = 640;
	CHECK_FALSE(missing.Field("Width", width));
	CHECK(width == 640);
	CHECK_FALSE(loading.HasErrors());
}

TEST_CASE("A child whose key is the wrong shape is reported") {
	Archive loading = Archive::Load("Rendering: 5");
	loading.Child("Rendering");
	REQUIRE(loading.Errors().size() == 1);
	CHECK(loading.Errors()[0].rfind("Rendering:", 0) == 0);
}

TEST_CASE("List round-trips structs") {
	struct Frame {
		i32 index = 0;
		f32 duration = 0.f;
	};
	const auto transferFrame = [](Archive &archive, Frame &frame) {
		archive.Field("index", frame.index);
		archive.Field("duration", frame.duration);
	};

	std::vector<Frame> frames{{1, 0.5f}, {2, 0.25f}, {3, 0.125f}};

	Archive saving = Archive::Save();
	saving.List("frames", frames, transferFrame);
	const std::string text = saving.ToString();

	std::vector<Frame> read;
	Archive loading = Archive::Load(text);
	REQUIRE(loading.List("frames", read, transferFrame));

	REQUIRE(read.size() == 3);
	CHECK(read[0].index == 1);
	CHECK(read[2].duration == doctest::Approx(0.125f));
	CHECK_FALSE(loading.HasErrors());
}

TEST_CASE("List round-trips plain scalars through Value") {
	const auto transferInt = [](Archive &archive, i32 &value) { archive.Value(value); };

	std::vector<i32> ids{4, 8, 15};

	Archive saving = Archive::Save();
	saving.List("ids", ids, transferInt);
	const std::string text = saving.ToString();
	CHECK(text.find("ids:") != std::string::npos);

	std::vector<i32> read{99, 99, 99, 99, 99};
	Archive loading = Archive::Load(text);
	REQUIRE(loading.List("ids", read, transferInt));

	// Resized down to the document's length, not merged into what was there.
	REQUIRE(read.size() == 3);
	CHECK(read[0] == 4);
	CHECK(read[1] == 8);
	CHECK(read[2] == 15);
}

TEST_CASE("A missing list leaves the vector alone; a malformed one is reported") {
	const auto transferInt = [](Archive &archive, i32 &value) { archive.Value(value); };

	std::vector<i32> ids{1, 2};
	Archive loading = Archive::Load("ids: 5");
	CHECK_FALSE(loading.List("ids", ids, transferInt));
	CHECK(ids.size() == 2);
	REQUIRE(loading.Errors().size() == 1);
	CHECK(loading.Errors()[0].rfind("ids:", 0) == 0);

	Archive absent = Archive::Load("other: 1");
	CHECK_FALSE(absent.List("ids", ids, transferInt));
	CHECK(ids.size() == 2);
	CHECK_FALSE(absent.HasErrors());
}

TEST_CASE("List element errors carry their index") {
	const auto transferInt = [](Archive &archive, i32 &value) { archive.Value(value); };

	std::vector<i32> ids;
	Archive loading = Archive::Load("ids: [1, nope, 3]");
	loading.List("ids", ids, transferInt);

	REQUIRE(loading.Errors().size() == 1);
	CHECK(loading.Errors()[0].rfind("ids[1]:", 0) == 0);
}

TEST_CASE("Enums are written by name, so renumbering doesn't break files") {
	Mode mode = Mode::Absolute;

	Archive saving = Archive::Save();
	saving.EnumField("mode", mode, modeConstants());
	const std::string text = saving.ToString();
	CHECK(text.find("mode: Absolute") != std::string::npos);

	Mode read = Mode::Relative;
	Archive loading = Archive::Load(text);
	CHECK(loading.EnumField("mode", read, modeConstants()));
	CHECK(read == Mode::Absolute);
}

TEST_CASE("A zero-valued enum constant round-trips by name") {
	Mode mode = Mode::Relative;

	Archive saving = Archive::Save();
	saving.EnumField("mode", mode, modeConstants());
	CHECK(saving.ToString().find("mode: Relative") != std::string::npos);
}

TEST_CASE("An unknown enum name is reported and keeps the current value") {
	Mode mode = Mode::Absolute;
	Archive loading = Archive::Load("mode: Sideways");
	CHECK_FALSE(loading.EnumField("mode", mode, modeConstants()));

	CHECK(mode == Mode::Absolute);
	REQUIRE(loading.Errors().size() == 1);
	CHECK(loading.Errors()[0].find("Sideways") != std::string::npos);
	CHECK(loading.Errors()[0].find("Relative, Absolute") != std::string::npos);
}

TEST_CASE("An enum value no constant covers round-trips as a number") {
	Mode mode = static_cast<Mode>(93);

	Archive saving = Archive::Save();
	saving.EnumField("mode", mode, modeConstants());
	const std::string text = saving.ToString();
	CHECK(text.find("mode: 93") != std::string::npos);

	Mode read = Mode::Relative;
	Archive loading = Archive::Load(text);
	CHECK(loading.EnumField("mode", read, modeConstants()));
	CHECK(static_cast<i32>(read) == 93);
}

TEST_CASE("Archive stamps and reads a format version") {
	Archive saving = Archive::Save();
	CHECK(saving.Version() == kArchiveVersion);
	CHECK(saving.ToString().find("version: 1") != std::string::npos);

	Archive loading = Archive::Load(saving.ToString());
	CHECK(loading.Version() == kArchiveVersion);

	// The version propagates into sub-archives, so a nested Serialize body can
	// branch on it too.
	CHECK(loading.Child("anything").Version() == kArchiveVersion);
}

TEST_CASE("A document written before versioning reads back as version 0") {
	Archive loading = Archive::Load("count: 1");
	REQUIRE(loading.IsValid());
	CHECK(loading.Version() == 0);
}

TEST_CASE("A malformed or non-mapping document is invalid, not silently empty") {
	Archive broken = Archive::Load("this: [is: not: yaml");
	CHECK_FALSE(broken.IsValid());
	CHECK(broken.HasErrors());

	Archive scalar = Archive::Load("just a string");
	CHECK_FALSE(scalar.IsValid());
	CHECK(scalar.HasErrors());

	Archive empty = Archive::Load("");
	CHECK_FALSE(empty.IsValid());

	// An invalid archive refuses reads instead of pretending everything is
	// absent, so a caller that ignores IsValid() still can't get junk.
	i32 value = 5;
	CHECK_FALSE(broken.Field("anything", value));
	CHECK(value == 5);
}

TEST_CASE("SaveToFile and LoadFromFile round-trip through the filesystem") {
	const auto root = makeTempDir("sowa_archive_test");
	FileSystem fs;
	fs.MountFolder("res", root);

	Settings written;
	written.title = "On disk";
	written.count = 11;
	written.position = Vector2(1.f, 2.f);

	REQUIRE(SaveToFile(fs, "res://settings.yaml", written).Ok());
	CHECK(readFile(root / "settings.yaml").find("title: On disk") != std::string::npos);

	Settings read;
	const ArchiveFileResult result = LoadFromFile(fs, "res://settings.yaml", read);
	REQUIRE(result.Ok());
	CHECK(read.title == "On disk");
	CHECK(read.count == 11);
	CHECK(read.position.y == doctest::Approx(2.f));

	std::filesystem::remove_all(root);
}

TEST_CASE("LoadFromFile separates a missing file from a corrupt one") {
	const auto root = makeTempDir("sowa_archive_missing_test");
	FileSystem fs;
	fs.MountFolder("res", root);

	Settings settings;

	const ArchiveFileResult missing = LoadFromFile(fs, "res://nope.yaml", settings);
	CHECK_FALSE(missing);
	CHECK(missing.error == FsError::NotFound);
	CHECK(missing.parsed);

	{
		std::ofstream out(root / "corrupt.yaml", std::ios::binary);
		out << "just a string";
	}
	const ArchiveFileResult corrupt = LoadFromFile(fs, "res://corrupt.yaml", settings);
	CHECK_FALSE(corrupt);
	CHECK(corrupt.error == FsError::Ok);
	CHECK_FALSE(corrupt.parsed);

	std::filesystem::remove_all(root);
}

TEST_CASE("A file that loads with problems still loads") {
	const auto root = makeTempDir("sowa_archive_partial_test");
	FileSystem fs;
	fs.MountFolder("res", root);

	{
		std::ofstream out(root / "partial.yaml", std::ios::binary);
		out << "title: kept\ncount: broken\n";
	}

	Settings settings;
	settings.count = 7;
	const ArchiveFileResult result = LoadFromFile(fs, "res://partial.yaml", settings);

	CHECK(static_cast<bool>(result));
	CHECK_FALSE(result.Ok());
	CHECK(result.problems.size() == 1);
	CHECK(settings.title == "kept");
	CHECK(settings.count == 7);

	std::filesystem::remove_all(root);
}
