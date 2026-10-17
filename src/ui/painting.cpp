#include "render/renderer2d.hpp"
#include "resource/font.hpp"
#include "resource/resource_registry.hpp"
#include "ui/drawing.hpp"
#include <algorithm>

namespace sowa {
namespace {
Color multiply(Color a, Color b) {
	return {a.r * b.r, a.g * b.g, a.b * b.b, a.a * b.a};
}
} // namespace

void UIPainter::Quad(Rect rect, Color color) {
	renderer.DrawQuad(rect.x, viewportHeight - rect.y - rect.h, rect.w, rect.h,
		multiply(color, tint), 0, token);
}

void UIPainter::Outline(Rect rect, Color color, f32 width) {
	Quad({rect.x, rect.y, rect.w, width}, color);
	Quad({rect.x, rect.Top() - width, rect.w, width}, color);
	Quad({rect.x, rect.y, width, rect.h}, color);
	Quad({rect.Right() - width, rect.y, width, rect.h}, color);
}

void UIPainter::Clip(Rect rect) {
	renderer.PushClip({rect.x, viewportHeight - rect.y - rect.h, rect.w, rect.h});
}

void UIPainter::PopClip() {
	renderer.PopClip();
}

void UIPainter::NinePatch(const UINinePatchRect &image, Rect rect) {
	DrawUINinePatchRect(renderer, resources, image, rect, viewportHeight, scale,
		multiply(image.modulate, tint), token);
}

void UIPainter::Image(const UIImage &image, Rect rect, Color color) {
	DrawUIImage(renderer, resources, image, rect, viewportHeight, scale, multiply(color, tint),
		token);
}

void UIPainter::Surface(Rect rect, Color color) {
	if (!style.texture) {
		Quad(rect, color);
		return;
	}
	UIImage image;
	image.texture = style.texture;
	image.mode = style.imageMode;
	image.regionPosition = style.regionPosition;
	image.regionSize = style.regionSize;
	image.borderMin = style.borderMin;
	image.borderMax = style.borderMax;
	image.tileCenter = style.tileCenter;
	image.tileEdges = style.tileEdges;
	Image(image, rect, color);
}

void UIPainter::Text(const std::string &value, Rect rect, const UITextPaint &paint) {
	const Font *font = defaultFont;
	if (paint.font) {
		font = resources.Get<Font>(paint.font).get();
	}
	if (!font || !font->IsLoaded() || rect.w <= 0 || rect.h <= 0) {
		return;
	}
	TextLayoutOptions options;
	options.fontSize = (paint.fontSize > 0 ? paint.fontSize : style.fontSize) * scale;
	options.wrapWidth = paint.wrap ? rect.w : 0;
	options.lineSpacing = paint.lineSpacing;
	options.horizontalAlignment = paint.alignment;
	options.verticalAlignment = TextVerticalAlignment::Center;
	options.offset = {rect.x + rect.w / 2, viewportHeight - rect.y - rect.h / 2};
	if (paint.alignment == TextHorizontalAlignment::Left) {
		options.offset.x = rect.x;
	}
	if (paint.alignment == TextHorizontalAlignment::Right) {
		options.offset.x = rect.Right();
	}
	if (paint.vertical == UIAlignment::Start) {
		options.verticalAlignment = TextVerticalAlignment::Top;
		options.offset.y = viewportHeight - rect.y;
	} else if (paint.vertical == UIAlignment::End) {
		options.verticalAlignment = TextVerticalAlignment::Bottom;
		options.offset.y = viewportHeight - rect.Top();
	}
	std::string text = value;
	auto block = LayoutTextBlock(*font, text, options);
	if (paint.overflow == UIOverflow::Ellipsis &&
		(block.bounds.w > rect.w || block.bounds.h > rect.h)) {
		size_t low = 0, high = text.size();
		while (low < high) {
			const size_t mid = (low + high + 1) / 2;
			const auto trial = LayoutTextBlock(*font, text.substr(0, mid) + "...", options);
			if (trial.bounds.w <= rect.w && trial.bounds.h <= rect.h) {
				low = mid;
			} else {
				high = mid - 1;
			}
		}
		while (low > 0 && low < text.size() && (static_cast<u8>(text[low]) & 0xc0) == 0x80) {
			--low;
		}
		text = text.substr(0, low) + "...";
		block = LayoutTextBlock(*font, text, options);
	}
	if (paint.overflow != UIOverflow::Visible) {
		renderer.PushClip({rect.x, viewportHeight - rect.y - rect.h, rect.w, rect.h});
	}
	const Color color = multiply(paint.color.value_or(textColor), tint);
	for (const auto &quad : block.quads) {
		renderer.DrawQuad(quad.rect.x, quad.rect.y, quad.rect.w, quad.rect.h, quad.uvRect,
			font->Atlas(), color, 0, token);
	}
	if (paint.overflow != UIOverflow::Visible) {
		renderer.PopClip();
	}
}

} // namespace sowa
