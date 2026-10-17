#include <string>
#include <vector>

#include <doctest/doctest.h>

#include "core/serialize/archive.hpp"
#include "resource/resource_registry.hpp"
#include "resource/sprite_sheet_animation.hpp"

using namespace sowa;

namespace {

struct Fixture {
	ResourceRegistry registry;
	Fixture() { registry.RegisterType<SpriteSheetAnimation>("SpriteSheetAnimation"); }

	Ref<SpriteSheetAnimation> Make() { return registry.Create<SpriteSheetAnimation>(); }
};

SpriteAnimation MakeWalk(ResourceID texture = ResourceID{7}) {
	SpriteAnimation walk;
	walk.texture = texture;
	walk.columns = 4;
	walk.rows = 2;
	walk.frames = {0, 1, 2, 3};
	walk.framesPerSecond = 8.f;
	walk.loop = true;
	return walk;
}

} // namespace

TEST_CASE("Animations are stored, found and replaced by name") {
	Fixture fixture;
	Ref<SpriteSheetAnimation> sheet = fixture.Make();

	CHECK(sheet->Set("walk", MakeWalk()));
	CHECK(sheet->Has("walk"));
	REQUIRE(sheet->Find("walk") != nullptr);
	CHECK(sheet->Find("walk")->framesPerSecond == doctest::Approx(8.f));

	SpriteAnimation faster = MakeWalk();
	faster.framesPerSecond = 24.f;
	CHECK(sheet->Set("walk", faster));
	CHECK(sheet->Count() == 1);
	CHECK(sheet->Find("walk")->framesPerSecond == doctest::Approx(24.f));

	CHECK(sheet->Remove("walk"));
	CHECK_FALSE(sheet->Remove("walk"));
	CHECK(sheet->Find("walk") == nullptr);
}

TEST_CASE("An unnamed animation is refused") {
	Fixture fixture;
	Ref<SpriteSheetAnimation> sheet = fixture.Make();

	// It would be unreachable by lookup and ambiguous in the editor's list.
	CHECK_FALSE(sheet->Set("", MakeWalk()));
	CHECK(sheet->Count() == 0);
}

TEST_CASE("Names come out sorted, so saves are byte-identical") {
	Fixture fixture;
	Ref<SpriteSheetAnimation> sheet = fixture.Make();

	sheet->Set("walk", MakeWalk());
	sheet->Set("idle", MakeWalk());
	sheet->Set("attack", MakeWalk());

	const std::vector<std::string> names = sheet->Names();
	REQUIRE(names.size() == 3);
	CHECK(names[0] == "attack");
	CHECK(names[1] == "idle");
	CHECK(names[2] == "walk");
}

TEST_CASE("Duration is frame count over rate, and zero when it can't advance") {
	SpriteAnimation walk = MakeWalk();
	CHECK(walk.Duration() == doctest::Approx(0.5f)); // 4 frames at 8 fps

	walk.framesPerSecond = 0.f;
	CHECK(walk.Duration() == doctest::Approx(0.f));

	walk.framesPerSecond = -8.f;
	CHECK(walk.Duration() == doctest::Approx(0.f));

	SpriteAnimation empty;
	CHECK(empty.Duration() == doctest::Approx(0.f));
}

// Grid rows run down from the top of the sheet; texture v runs up from the
// bottom. Getting the flip backwards plays the animation with its rows
// reversed - obvious on screen, invisible to any check of the rect's size.
TEST_CASE("The first cell is the top-left of the sheet, not the bottom-left") {
	SpriteAnimation anim = MakeWalk();
	anim.frames = {0};

	const Rect uv = anim.FrameUV(0);
	CHECK(uv.x == doctest::Approx(0.f));
	CHECK(uv.w == doctest::Approx(0.25f)); // 4 columns
	CHECK(uv.h == doctest::Approx(0.5f));  // 2 rows
	// Top row, so it occupies the upper half in texture coordinates.
	CHECK(uv.y == doctest::Approx(0.5f));
	CHECK(uv.y + uv.h == doctest::Approx(1.f));
}

TEST_CASE("Cell indices are row-major from the top-left") {
	SpriteAnimation anim = MakeWalk();
	anim.frames = {0, 3, 4, 7};

	// Cell 3: last column of the top row.
	const Rect topRight = anim.FrameUV(1);
	CHECK(topRight.x == doctest::Approx(0.75f));
	CHECK(topRight.y == doctest::Approx(0.5f));

	// Cell 4: wraps to the first column of the *second* row, which is lower
	// on the sheet and therefore lower in v.
	const Rect secondRow = anim.FrameUV(2);
	CHECK(secondRow.x == doctest::Approx(0.f));
	CHECK(secondRow.y == doctest::Approx(0.f));

	// Cell 7: last column of the second row.
	const Rect bottomRight = anim.FrameUV(3);
	CHECK(bottomRight.x == doctest::Approx(0.75f));
	CHECK(bottomRight.y == doctest::Approx(0.f));
}

TEST_CASE("Every frame's uv stays inside the texture") {
	SpriteAnimation anim = MakeWalk();
	anim.frames = {0, 1, 2, 3, 4, 5, 6, 7};

	for (i32 i = 0; i < anim.FrameCount(); ++i) {
		const Rect uv = anim.FrameUV(i);
		CHECK(uv.x >= 0.f);
		CHECK(uv.y >= 0.f);
		CHECK(uv.x + uv.w <= doctest::Approx(1.f));
		CHECK(uv.y + uv.h <= doctest::Approx(1.f));
	}
}

TEST_CASE("A frame list may repeat cells and skip around") {
	SpriteAnimation anim = MakeWalk();
	anim.frames = {2, 0, 2};

	CHECK(anim.FrameCount() == 3);
	// Entries 0 and 2 are the same cell, so they map to the same region.
	CHECK(anim.FrameUV(0).x == doctest::Approx(anim.FrameUV(2).x));
	CHECK(anim.FrameUV(0).y == doctest::Approx(anim.FrameUV(2).y));
	CHECK(anim.FrameUV(1).x == doctest::Approx(0.f));
}

TEST_CASE("An out-of-range index or cell yields an empty rect, not garbage") {
	SpriteAnimation anim = MakeWalk();

	CHECK(anim.FrameUV(-1).w == doctest::Approx(0.f));
	CHECK(anim.FrameUV(99).w == doctest::Approx(0.f));

	// A cell past the end of the grid: 4x2 has cells 0..7.
	anim.frames = {8};
	CHECK(anim.FrameUV(0).w == doctest::Approx(0.f));

	anim.frames = {-1};
	CHECK(anim.FrameUV(0).w == doctest::Approx(0.f));

	// A degenerate grid can't be divided at all.
	SpriteAnimation broken = MakeWalk();
	broken.columns = 0;
	CHECK(broken.FrameUV(0).w == doctest::Approx(0.f));
}

TEST_CASE("IsValid wants a texture, a grid and at least one frame") {
	CHECK(MakeWalk().IsValid());

	SpriteAnimation noTexture = MakeWalk();
	noTexture.texture = ResourceID{};
	CHECK_FALSE(noTexture.IsValid());

	SpriteAnimation noFrames = MakeWalk();
	noFrames.frames.clear();
	CHECK_FALSE(noFrames.IsValid());

	SpriteAnimation noGrid = MakeWalk();
	noGrid.rows = 0;
	CHECK_FALSE(noGrid.IsValid());
}

TEST_CASE("A sheet round-trips every animation through an archive") {
	Fixture fixture;
	Ref<SpriteSheetAnimation> sheet = fixture.Make();

	SpriteAnimation walk = MakeWalk(ResourceID{0xABCD'1234'5678'9EF0ull});
	SpriteAnimation die = MakeWalk(ResourceID{99});
	die.frames = {4, 5, 6};
	die.framesPerSecond = 6.f;
	die.loop = false;
	die.columns = 3;
	die.rows = 3;

	sheet->Set("walk", walk);
	sheet->Set("die", die);

	Archive saving = Archive::Save();
	sheet->Serialize(saving);

	Ref<SpriteSheetAnimation> restored = fixture.Make();
	Archive loading = Archive::Load(saving.ToString());
	REQUIRE(loading.IsValid());
	restored->Serialize(loading);
	CHECK_FALSE(loading.HasErrors());

	REQUIRE(restored->Count() == 2);

	const SpriteAnimation *restoredWalk = restored->Find("walk");
	REQUIRE(restoredWalk != nullptr);
	// The full 64 bits of the reference have to survive - a narrowed id points
	// the animation at a texture that doesn't exist.
	CHECK(restoredWalk->texture.value == 0xABCD'1234'5678'9EF0ull);
	CHECK(restoredWalk->columns == 4);
	CHECK(restoredWalk->rows == 2);
	CHECK(restoredWalk->frames == std::vector<i32>{0, 1, 2, 3});
	CHECK(restoredWalk->framesPerSecond == doctest::Approx(8.f));
	CHECK(restoredWalk->loop);

	const SpriteAnimation *restoredDie = restored->Find("die");
	REQUIRE(restoredDie != nullptr);
	CHECK(restoredDie->frames == std::vector<i32>{4, 5, 6});
	CHECK(restoredDie->framesPerSecond == doctest::Approx(6.f));
	CHECK_FALSE(restoredDie->loop);
	CHECK(restoredDie->columns == 3);
}

TEST_CASE("Loading replaces what was there rather than merging into it") {
	Fixture fixture;
	Ref<SpriteSheetAnimation> sheet = fixture.Make();
	sheet->Set("walk", MakeWalk());

	Archive saving = Archive::Save();
	sheet->Serialize(saving);

	Ref<SpriteSheetAnimation> target = fixture.Make();
	target->Set("stale", MakeWalk());
	target->Set("walk", MakeWalk());

	Archive loading = Archive::Load(saving.ToString());
	target->Serialize(loading);

	// "stale" was not in the document, so it must be gone - a load that merged
	// would leave animations from whatever the object held before.
	CHECK(target->Count() == 1);
	CHECK(target->Find("stale") == nullptr);
	CHECK(target->Has("walk"));
}

TEST_CASE("An empty sheet round-trips as empty") {
	Fixture fixture;
	Ref<SpriteSheetAnimation> sheet = fixture.Make();

	Archive saving = Archive::Save();
	sheet->Serialize(saving);

	Ref<SpriteSheetAnimation> restored = fixture.Make();
	restored->Set("leftover", MakeWalk());
	Archive loading = Archive::Load(saving.ToString());
	restored->Serialize(loading);

	CHECK(restored->Count() == 0);
}

TEST_CASE("An animation record with no name is reported, not silently kept") {
	Fixture fixture;
	Ref<SpriteSheetAnimation> sheet = fixture.Make();

	Archive loading = Archive::Load(
		"animations:\n"
		"  - name: \"\"\n"
		"    columns: 2\n"
		"    rows: 1\n"
		"    frames: [0, 1]\n"
		"  - name: good\n"
		"    columns: 2\n"
		"    rows: 1\n"
		"    frames: [0]\n");
	REQUIRE(loading.IsValid());
	sheet->Serialize(loading);

	CHECK(sheet->Count() == 1);
	CHECK(sheet->Has("good"));
	CHECK(loading.HasErrors());
}
