#pragma once

#include "core/types.hpp"

// UTF-8 Font Awesome solid glyphs. Keep LoadEditorFonts' glyph range in sync when adding icons.

#define ICON_FA_XMARK "\xef\x80\x8d"		 // U+F00D
#define ICON_FA_PEN_TO_SQUARE "\xef\x81\x84" // U+F044
#define ICON_FA_PLAY "\xef\x81\x8b"			 // U+F04B
#define ICON_FA_PAUSE "\xef\x81\x8c"		 // U+F04C
#define ICON_FA_STOP "\xef\x81\x8d"			 // U+F04D
#define ICON_FA_FORWARD_STEP "\xef\x81\x91"	 // U+F051
#define ICON_FA_ARROW_UP "\xef\x81\xa2"		 // U+F062
#define ICON_FA_ARROW_DOWN "\xef\x81\xa3"	 // U+F063
#define ICON_FA_PLUS "\xef\x81\xa7"			 // U+F067
#define ICON_FA_FOLDER "\xef\x81\xbb"		 // U+F07B
#define ICON_FA_FOLDER_OPEN "\xef\x81\xbc"	 // U+F07C
#define ICON_FA_FILE "\xef\x85\x9b"			 // U+F15B
#define ICON_FA_FILE_CODE "\xef\x87\x89"	 // U+F1C9
#define ICON_FA_DATABASE "\xef\x87\x80"		 // U+F1C0
#define ICON_FA_SITEMAP "\xef\x83\xa8"		 // U+F0E8
#define ICON_FA_CUBE "\xef\x86\xb2"			 // U+F1B2
#define ICON_FA_SLIDERS "\xef\x87\x9e"		 // U+F1DE
#define ICON_FA_TRASH "\xef\x87\xb8"		 // U+F1F8
#define ICON_FA_TERMINAL "\xef\x84\xa0"		 // U+F120
#define ICON_FA_ARROWS "\xef\x81\x87"		 // U+F047
#define ICON_FA_ROTATE_RIGHT "\xef\x80\x9e"	 // U+F01E
#define ICON_FA_EXPAND "\xef\x81\xa5"		 // U+F065
#define ICON_FA_EYE_SLASH "\xef\x81\xb0"	 // U+F070

#define ICON_FA_CAMERA "\xef\x80\xb0"			   // U+F030
#define ICON_FA_IMAGE "\xef\x80\xbe"			   // U+F03E
#define ICON_FA_FILM "\xef\x80\x88"				   // U+F008
#define ICON_FA_MUSIC "\xef\x80\x81"			   // U+F001
#define ICON_FA_FONT "\xef\x80\xb1"				   // U+F031
#define ICON_FA_LAYER_GROUP "\xef\x97\xbd"		   // U+F5FD
#define ICON_FA_WAND_MAGIC_SPARKLES "\xef\x83\x90" // U+F0D0
#define ICON_FA_LINK "\xef\x83\x81"				   // U+F0C1
#define ICON_FA_KEYBOARD "\xef\x84\x9c"			   // U+F11C
#define ICON_FA_TRIANGLE_EXCLAMATION "\xef\x81\xb1" // U+F071

namespace sowa::editor {

inline constexpr u32 kIconMin = 0xF001;
inline constexpr u32 kIconMax = 0xF5FD;
inline constexpr u32 kIconFolder = 0xF07B;
inline constexpr u32 kIconXmark = 0xF00D;

} // namespace sowa::editor
