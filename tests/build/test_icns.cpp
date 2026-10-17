#include <doctest/doctest.h>

#include <cstdint>
#include <string>
#include <vector>

#include <miniz.h>
#include <stb_image.h>

#include "build/icns.hpp"

using namespace sowa::build;

namespace {

std::uint32_t readU32(const std::string &data, std::size_t offset) {
	return (static_cast<std::uint32_t>(static_cast<unsigned char>(data[offset])) << 24) |
		   (static_cast<std::uint32_t>(static_cast<unsigned char>(data[offset + 1])) << 16) |
		   (static_cast<std::uint32_t>(static_cast<unsigned char>(data[offset + 2])) << 8) |
		   static_cast<std::uint32_t>(static_cast<unsigned char>(data[offset + 3]));
}

std::string png(int width, int height) {
	std::vector<unsigned char> pixels(static_cast<std::size_t>(width) * height * 4, 200);
	std::size_t length = 0;
	void *data = tdefl_write_image_to_png_file_in_memory_ex(pixels.data(), width, height, 4,
		&length, 6, false);
	std::string bytes(static_cast<const char *>(data), length);
	mz_free(data);
	return bytes;
}

} // namespace

TEST_CASE("icns files hold one PNG per standard icon size") {
	std::string error;
	const auto icns = MakeIcns(png(40, 30), error);
	REQUIRE_MESSAGE(icns, error);
	CHECK(icns->substr(0, 4) == "icns");
	CHECK(readU32(*icns, 4) == icns->size());

	std::vector<std::string> types;
	for (std::size_t offset = 8; offset < icns->size();) {
		const std::uint32_t length = readU32(*icns, offset + 4);
		types.push_back(icns->substr(offset, 4));
		CHECK(icns->substr(offset + 9, 3) == "PNG");
		offset += length;
	}
	CHECK(types == std::vector<std::string>{"icp4", "ic11", "icp5", "ic12", "ic07", "ic13", "ic08",
					   "ic14", "ic09", "ic10"});
}

TEST_CASE("icns generation rejects data that is not an image") {
	std::string error;
	CHECK_FALSE(MakeIcns("not an image", error));
	CHECK(error.find("cannot decode") == 0);
}

TEST_CASE("icns images stay upright even after textures enabled stb's vertical flip") {
	// 2x2: top row red, bottom row blue.
	const unsigned char pixels[] = {255, 0, 0, 255, 255, 0, 0, 255, 0, 0, 255, 255, 0, 0, 255, 255};
	std::size_t length = 0;
	void *data = tdefl_write_image_to_png_file_in_memory_ex(pixels, 2, 2, 4, &length, 6, false);
	const std::string source(static_cast<const char *>(data), length);
	mz_free(data);

	stbi_set_flip_vertically_on_load(1);
	std::string error;
	const auto icns = MakeIcns(source, error);
	REQUIRE_MESSAGE(icns, error);

	// The first entry (icp4, 16x16) starts after the 8-byte file header.
	const std::uint32_t entryLength = readU32(*icns, 12);
	stbi_set_flip_vertically_on_load(0);
	int width = 0, height = 0, channels = 0;
	stbi_uc *icon = stbi_load_from_memory(reinterpret_cast<const stbi_uc *>(icns->data() + 16),
		static_cast<int>(entryLength - 8), &width, &height, &channels, 4);
	REQUIRE(icon != nullptr);
	CHECK(width == 16);
	CHECK(icon[0] == 255); // top-left is red
	CHECK(icon[2] == 0);
	CHECK(icon[(15 * 16) * 4 + 2] == 255); // bottom-left is blue
	stbi_image_free(icon);
}
