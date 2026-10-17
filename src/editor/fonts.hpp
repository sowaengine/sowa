#pragma once

struct ImFontAtlas;

namespace sowa::editor {

// Uses fonts embedded in the executable; requires no filesystem mounts.
bool LoadEmbeddedEditorFonts(ImFontAtlas &atlas);

// Adds Roboto as the default font and merges Font Awesome solid into it.
// Caller-owned TTF buffers must outlive the atlas.
// Returns false if the text font cannot be added.
bool LoadEditorFonts(ImFontAtlas &atlas, const void *textTtf, int textBytes, const void *iconTtf,
	int iconBytes);

} // namespace sowa::editor
