#include <doctest/doctest.h>

#include <string>

#include "editor/fonts.hpp"
#include "editor/icons.hpp"

#include "imgui.h"

using namespace sowa;

TEST_CASE("LoadEditorFonts returns false for empty text font") {
	ImGui::CreateContext();
	CHECK_FALSE(editor::LoadEditorFonts(*ImGui::GetIO().Fonts, nullptr, 0, nullptr, 0));
	ImGui::DestroyContext();
}

TEST_CASE("Embedded editor fonts supply Roboto and Font Awesome without filesystem mounts") {
	ImGui::CreateContext();
	ImFontAtlas &atlas = *ImGui::GetIO().Fonts;
	REQUIRE(editor::LoadEmbeddedEditorFonts(atlas));

	unsigned char *pixels = nullptr;
	int width = 0;
	int height = 0;
	atlas.GetTexDataAsRGBA32(&pixels, &width, &height);
	REQUIRE(pixels != nullptr);
	REQUIRE(width > 0);
	REQUIRE(height > 0);
	REQUIRE(atlas.Fonts.Size >= 1);

	ImFont *font = atlas.Fonts[0];
	REQUIRE(font != nullptr);
	CHECK(std::string(font->GetDebugName()) == "Roboto Regular");
	REQUIRE(atlas.Sources.Size == 2);
	CHECK(std::string(atlas.Sources[1].Name) == "Font Awesome Solid");
	// 1.92: glyphs live on ImFontBaked, not ImFont. IsGlyphInFont is the
	// atlas-level query and goes red if the icon merge never ran.
	CHECK(font->IsGlyphInFont(static_cast<ImWchar>('A')));
	CHECK(font->IsGlyphInFont(static_cast<ImWchar>('a')));
	CHECK(font->IsGlyphInFont(static_cast<ImWchar>(editor::kIconFolder)));
	CHECK(font->IsGlyphInFont(static_cast<ImWchar>(editor::kIconXmark)));
	// Move, rotate, resize and hide buttons use the existing merged icon font.
	for (ImWchar icon : {0xF067, 0xF07C, 0xF1B2, 0xF1F8, 0xF047, 0xF01E, 0xF065, 0xF070, 0xF030,
			 0xF03E, 0xF008, 0xF001, 0xF031, 0xF5FD, 0xF0D0, 0xF0C1}) {
		CHECK(font->IsGlyphInFont(icon));
	}
	CHECK_FALSE(font->IsGlyphInFont(static_cast<ImWchar>(0x4E2D)));

	ImGui::DestroyContext();
}
