#include "build/icns.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <vector>

#include <miniz.h>
#include <stb_image.h>

namespace sowa::build {
namespace {

struct IconSize {
	const char *type;
	int size;
};

// The entries iconutil writes for a full .iconset: each point size at 1x
// and 2x (Retina), all stored as PNG.
constexpr std::array<IconSize, 10> kSizes = {{
	{"icp4", 16},	// 16x16
	{"ic11", 32},	// 16x16@2x
	{"icp5", 32},	// 32x32
	{"ic12", 64},	// 32x32@2x
	{"ic07", 128},	// 128x128
	{"ic13", 256},	// 128x128@2x
	{"ic08", 256},	// 256x256
	{"ic14", 512},	// 256x256@2x
	{"ic09", 512},	// 512x512
	{"ic10", 1024}, // 512x512@2x
}};

void appendU32(std::string &out, std::uint32_t value) {
	out += static_cast<char>((value >> 24) & 0xff);
	out += static_cast<char>((value >> 16) & 0xff);
	out += static_cast<char>((value >> 8) & 0xff);
	out += static_cast<char>(value & 0xff);
}

// Box-filters a square RGBA image to size, averaging in premultiplied
// alpha so transparent edges don't darken. Upscaling repeats pixels.
std::vector<unsigned char> scale(const std::vector<unsigned char> &source, int sourceSize,
	int size) {
	std::vector<unsigned char> out(static_cast<std::size_t>(size) * size * 4);
	for (int y = 0; y < size; ++y) {
		const int y0 = y * sourceSize / size;
		const int y1 = std::max(y0 + 1, (y + 1) * sourceSize / size);
		for (int x = 0; x < size; ++x) {
			const int x0 = x * sourceSize / size;
			const int x1 = std::max(x0 + 1, (x + 1) * sourceSize / size);
			double r = 0, g = 0, b = 0, a = 0;
			for (int sy = y0; sy < y1; ++sy) {
				for (int sx = x0; sx < x1; ++sx) {
					const unsigned char *p =
						&source[(static_cast<std::size_t>(sy) * sourceSize + sx) * 4];
					const double alpha = p[3] / 255.0;
					r += p[0] * alpha;
					g += p[1] * alpha;
					b += p[2] * alpha;
					a += p[3];
				}
			}
			const double count = static_cast<double>((y1 - y0) * (x1 - x0));
			unsigned char *q = &out[(static_cast<std::size_t>(y) * size + x) * 4];
			const double alpha = a / count;
			const double unpremultiply = alpha > 0 ? 255.0 / alpha : 0.0;
			q[0] = static_cast<unsigned char>(std::min(255.0, r / count * unpremultiply + 0.5));
			q[1] = static_cast<unsigned char>(std::min(255.0, g / count * unpremultiply + 0.5));
			q[2] = static_cast<unsigned char>(std::min(255.0, b / count * unpremultiply + 0.5));
			q[3] = static_cast<unsigned char>(alpha + 0.5);
		}
	}
	return out;
}

} // namespace

std::optional<std::string> MakeIcns(std::string_view image, std::string &error) {
	int width = 0;
	int height = 0;
	int channels = 0;
	stbi_set_flip_vertically_on_load(0);
	stbi_uc *pixels = stbi_load_from_memory(reinterpret_cast<const stbi_uc *>(image.data()),
		static_cast<int>(image.size()), &width, &height, &channels, 4);
	if (pixels == nullptr) {
		error = std::string("cannot decode the icon: ") + stbi_failure_reason();
		return std::nullopt;
	}

	// Center on a transparent square so non-square icons keep their aspect.
	const int side = std::max(width, height);
	std::vector<unsigned char> square(static_cast<std::size_t>(side) * side * 4, 0);
	const int offsetX = (side - width) / 2;
	const int offsetY = (side - height) / 2;
	for (int y = 0; y < height; ++y) {
		std::copy_n(pixels + static_cast<std::size_t>(y) * width * 4, width * 4,
			&square[(static_cast<std::size_t>(y + offsetY) * side + offsetX) * 4]);
	}
	stbi_image_free(pixels);

	std::string entries;
	for (const IconSize &icon : kSizes) {
		const auto scaled = scale(square, side, icon.size);
		std::size_t length = 0;
		void *png = tdefl_write_image_to_png_file_in_memory_ex(scaled.data(), icon.size, icon.size,
			4, &length, 6, false);
		if (png == nullptr) {
			error = "cannot encode the icon as PNG";
			return std::nullopt;
		}
		entries.append(icon.type, 4);
		appendU32(entries, static_cast<std::uint32_t>(length + 8));
		entries.append(static_cast<const char *>(png), length);
		mz_free(png);
	}

	std::string icns = "icns";
	appendU32(icns, static_cast<std::uint32_t>(entries.size() + 8));
	icns += entries;
	return icns;
}

} // namespace sowa::build
