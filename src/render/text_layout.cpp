#include "render/text_layout.hpp"

#include <algorithm>
#include <cmath>
#include <string>

#include "render/text_draw.hpp"

namespace sowa {

TextLayout LayoutTextBlock(const Font &font, std::string_view text,
	const TextLayoutOptions &options) {
	TextLayout result;
	if (!font.HasGlyphs() || text.empty() || !std::isfinite(options.fontSize) ||
		options.fontSize < 0.f || !std::isfinite(options.wrapWidth) || options.wrapWidth < 0.f ||
		!std::isfinite(options.lineSpacing) || options.lineSpacing <= 0.f ||
		!std::isfinite(options.offset.x) || !std::isfinite(options.offset.y)) {
		return result;
	}

	const f32 scale =
		options.fontSize == 0.f ? 1.f : options.fontSize / (font.Ascent() - font.Descent());
	if (!std::isfinite(scale) || scale <= 0.f) {
		return result;
	}
	const f32 wrap = options.wrapWidth / scale;
	std::string normalized;
	for (size_t i = 0; i < text.size(); ++i) {
		const char c = text[i];
		if (c == '\r') {
			normalized += '\n';
			if (i + 1 < text.size() && text[i + 1] == '\n') {
				++i;
			}
		} else if (c == '\t') {
			normalized += "    ";
		} else if (c == '\n' || font.FindGlyph(static_cast<u8>(c))) {
			normalized += c;
		}
	}

	struct Line {
		std::string_view text;
		f32 width;
	};

	std::vector<Line> lines;
	size_t start = 0;
	while (start <= normalized.size()) {
		f32 width = 0.f;
		size_t end = start;
		size_t space = std::string::npos;
		f32 widthBeforeSpace = 0.f;
		bool wrapped = false;
		for (; end < normalized.size() && normalized[end] != '\n'; ++end) {
			const f32 advance = font.FindGlyph(static_cast<u8>(normalized[end]))->advance;
			if (normalized[end] == ' ' && (end == start || normalized[end - 1] != ' ')) {
				space = end;
				widthBeforeSpace = width;
			}
			if (wrap > 0.f && width + advance > wrap && end > start) {
				if (space != std::string::npos && space > start) {
					end = space;
					width = widthBeforeSpace;
				}
				wrapped = true;
				break;
			}
			width += advance;
		}
		lines.push_back({std::string_view(normalized).substr(start, end - start), width});
		if (end == normalized.size()) {
			break;
		}
		start = end;
		if (wrapped) {
			while (start < normalized.size() && normalized[start] == ' ') {
				++start;
			}
			if (start == normalized.size()) {
				break;
			}
			// A newline after discarded wrap spaces ends this same line.
			if (normalized[start] == '\n') {
				++start;
			}
		} else {
			++start; // explicit newline, including a final empty line
		}
	}

	const f32 step = font.LineHeight() * options.lineSpacing * scale;
	const f32 top = font.Ascent() * scale;
	const f32 bottom = font.Descent() * scale - step * (lines.size() - 1);
	f32 baseline = options.offset.y;
	switch (options.verticalAlignment) {
	case TextVerticalAlignment::Baseline:
		break;
	case TextVerticalAlignment::Top:
		baseline -= top;
		break;
	case TextVerticalAlignment::Center:
		baseline -= (top + bottom) * 0.5f;
		break;
	case TextVerticalAlignment::Bottom:
		baseline -= bottom;
		break;
	}
	f32 alignment = 0.f;
	if (options.horizontalAlignment == TextHorizontalAlignment::Center) {
		alignment = 0.5f;
	} else if (options.horizontalAlignment == TextHorizontalAlignment::Right) {
		alignment = 1.f;
	}
	f32 widest = 0.f;
	for (size_t i = 0; i < lines.size(); ++i) {
		const auto &line = lines[i];
		const f32 width = line.width * scale;
		widest = std::max(widest, width);
		const size_t first = result.quads.size();
		LayoutText(font, line.text, {0.f, 0.f}, result.quads);
		for (size_t j = first; j < result.quads.size(); ++j) {
			auto &rect = result.quads[j].rect;
			rect = Rect(rect.x * scale + options.offset.x - width * alignment,
				rect.y * scale + baseline - step * i, rect.w * scale, rect.h * scale);
		}
	}
	result.bounds =
		Rect(options.offset.x - widest * alignment, baseline + bottom, widest, top - bottom);
	result.lineCount = static_cast<u32>(lines.size());
	return result;
}

} // namespace sowa
