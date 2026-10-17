#include "editor/fonts.hpp"

#include "assets/embedded.hpp"
#include "editor/icons.hpp"
#include <cstdio>

#include "imgui.h"

namespace sowa::editor {

bool LoadEmbeddedEditorFonts(ImFontAtlas &atlas) {
	const auto text = assets::DefaultFont();
	const auto icons = assets::EditorIcons();
	return LoadEditorFonts(atlas, text.data(), static_cast<int>(text.size()), icons.data(),
		static_cast<int>(icons.size()));
}

bool LoadEditorFonts(ImFontAtlas &atlas, const void *textTtf, int textBytes, const void *iconTtf,
	int iconBytes) {
	if (textTtf == nullptr || textBytes <= 0) {
		return false;
	}

	ImFontConfig textCfg;
	std::snprintf(textCfg.Name, sizeof(textCfg.Name), "Roboto Regular");
	textCfg.OversampleH = 2;
	textCfg.OversampleV = 2;
	textCfg.PixelSnapH = true;
	textCfg.FontDataOwnedByAtlas = false;
	if (atlas.AddFontFromMemoryTTF(const_cast<void *>(textTtf), textBytes, 16.f, &textCfg) ==
		nullptr) {
		return false;
	}

	if (iconTtf == nullptr || iconBytes <= 0) {
		return true;
	}

	static const ImWchar kIconRanges[] = {
		static_cast<ImWchar>(kIconMin),
		static_cast<ImWchar>(kIconMax),
		0,
	};
	ImFontConfig iconCfg;
	std::snprintf(iconCfg.Name, sizeof(iconCfg.Name), "Font Awesome Solid");
	iconCfg.MergeMode = true;
	iconCfg.PixelSnapH = true;
	iconCfg.GlyphMinAdvanceX = 14.f;
	iconCfg.FontDataOwnedByAtlas = false;
	return atlas.AddFontFromMemoryTTF(const_cast<void *>(iconTtf), iconBytes, 14.f, &iconCfg,
			   kIconRanges) != nullptr;
}

} // namespace sowa::editor
