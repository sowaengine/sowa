#include "render/image_patches.hpp"
#include <doctest/doctest.h>

using namespace sowa;

namespace {
void checkRect(Rect actual, Rect expected) {
	CHECK(actual.x == doctest::Approx(expected.x));
	CHECK(actual.y == doctest::Approx(expected.y));
	CHECK(actual.w == doctest::Approx(expected.w));
	CHECK(actual.h == doctest::Approx(expected.h));
}
} // namespace

TEST_CASE("Nine-patch: resizing preserves asymmetric corners and atlas orientation") {
	const ImagePatchOptions options{
		.regionPosition = {8, 10},
		.regionSize = {32, 40},
		.paddingLeft = 4,
		.paddingRight = 8,
		.paddingTop = 6,
		.paddingBottom = 10,
		.ninePatch = true,
	};
	for (bool flipped : {false, true}) {
		const auto patches = BuildImagePatches({20, 30, 100, 80}, {64, 128}, flipped, options);
		REQUIRE(patches.size() == 9);
		checkRect(patches.front().rect, Rect(20, 30, 4, 6));
		checkRect(patches[4].rect, Rect(24, 36, 88, 64));
		checkRect(patches.back().rect, Rect(112, 100, 8, 10));
		CHECK(patches.front().uv.x == doctest::Approx(8.f / 64));
		CHECK(patches.front().uv.w == doctest::Approx(4.f / 64));
		CHECK(patches.front().uv.y == doctest::Approx(flipped ? 1 - 16.f / 128 : 16.f / 128));
		CHECK(patches.front().uv.h == doctest::Approx((flipped ? 6.f : -6.f) / 128));
	}
}

TEST_CASE("Nine-patch: undersized rectangles shrink borders without inverted quads") {
	const ImagePatchOptions options{
		.paddingLeft = 4,
		.paddingRight = 8,
		.paddingTop = 6,
		.paddingBottom = 10,
		.ninePatch = true,
	};
	const auto patches = BuildImagePatches({0, 0, 6, 8}, {32, 32}, true, options);
	REQUIRE(patches.size() == 4);
	checkRect(patches.front().rect, Rect(0, 0, 2, 3));
	checkRect(patches.back().rect, Rect(2, 3, 4, 5));
	float area = 0;
	for (const auto &patch : patches) {
		CHECK(patch.rect.w > 0);
		CHECK(patch.rect.h > 0);
		area += patch.rect.w * patch.rect.h;
	}
	CHECK(area == 48);
}

TEST_CASE("Nine-patch: tiled centers crop the last tile and hollow panels omit it") {
	ImagePatchOptions options{.paddingLeft = 2,
		.paddingRight = 2,
		.paddingTop = 2,
		.paddingBottom = 2,
		.ninePatch = true,
		.tileCenter = true};
	const auto patches = BuildImagePatches({0, 0, 17, 9}, {10, 10}, true, options);
	REQUIRE(patches.size() == 11); // Eight border pieces and three center tiles.
	checkRect(patches[5].rect, Rect(8, 2, 6, 5));
	checkRect(patches[6].rect, Rect(14, 2, 1, 5));
	CHECK(patches[6].uv.w == doctest::Approx(.1f));
	CHECK(patches[6].uv.h == doctest::Approx(.5f));
	options.drawCenter = false;
	CHECK(BuildImagePatches({0, 0, 17, 9}, {10, 10}, true, options).size() == 8);
	options.tileEdges = true;
	const auto hollow = BuildImagePatches({0, 0, 17, 9}, {10, 10}, true, options);
	CHECK(hollow.size() == 12); // Three tiles on each horizontal edge.
	for (const auto &patch : hollow) {
		CHECK((patch.rect.y == 0 || patch.rect.y == 7 || patch.rect.x == 0 || patch.rect.x == 15));
	}
}

TEST_CASE("Nine-patch: clipping bounds tile generation and preserves its phase") {
	const ImagePatchOptions options{
		.tileCenter = true,
	};
	const auto patches = BuildImagePatches({0, 0, 1000000, 1000000}, {16, 16}, true, options, 2,
										   Rect(35, 67, 10, 10));
	REQUIRE(patches.size() == 1);
	checkRect(patches[0].rect, Rect(32, 64, 32, 32));
	const auto scaled = BuildImagePatches({0, 0, 80, 60}, {32, 32}, true,
		{.paddingLeft = 3,
			.paddingRight = 5,
			.paddingTop = 4,
			.paddingBottom = 6,
			.ninePatch = true},
		2);
	REQUIRE(scaled.size() == 9);
	checkRect(scaled.front().rect, Rect(0, 0, 6, 8));
	checkRect(scaled.back().rect, Rect(70, 48, 10, 12));
}

TEST_CASE("Nine-patch: empty regions and invalid sizes produce no geometry") {
	ImagePatchOptions options{.regionPosition = {64, 0}, .ninePatch = true};
	CHECK(BuildImagePatches({0, 0, 100, 100}, {64, 64}, true, options).empty());
	options.regionPosition = {0, 0};
	CHECK(BuildImagePatches({0, 0, 0, 100}, {64, 64}, true, options).empty());
	CHECK(BuildImagePatches({0, 0, 100, 100}, {0, 64}, true, options).empty());
	options.paddingLeft = options.paddingTop = -5;
	options.paddingRight = options.paddingBottom = 100;
	const auto patches = BuildImagePatches({0, 0, 100, 100}, {64, 64}, true, options);
	REQUIRE(patches.size() == 1);
	checkRect(patches[0].rect, Rect(36, 36, 64, 64));
}
