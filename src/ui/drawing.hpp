#pragma once
#include "ui/theme.hpp"
#include <optional>

namespace sowa {
class Renderer2D;
class ResourceRegistry;
class Font;

struct UITextPaint {
	TextHorizontalAlignment alignment = TextHorizontalAlignment::Center;
	std::optional<Color> color;
	bool wrap = false;
	UIOverflow overflow = UIOverflow::Clip;
	ResourceID font;
	f32 fontSize = 0;
	UIAlignment vertical = UIAlignment::Center;
	f32 lineSpacing = 1;
};

// Per-control drawing context. Coordinates stay in canvas space; the canvas
// projection handles placement, rotation and zoom for every primitive.
struct UIPainter {
	Renderer2D &renderer;
	ResourceRegistry &resources;
	const UIAppearance &style;
	const Font *defaultFont;
	f32 viewportHeight;
	f32 scale;
	Color tint;
	Color textColor;
	u32 token;

	void Quad(Rect rect, Color color);
	void Outline(Rect rect, Color color, f32 width);
	void Surface(Rect rect, Color color);
	void Image(const UIImage &image, Rect rect, Color color);
	void NinePatch(const UINinePatchRect &image, Rect rect);
	void Text(const std::string &text, Rect rect, const UITextPaint &options = {});
	void Clip(Rect rect);
	void PopClip();
};

void DrawUIImage(Renderer2D &renderer, ResourceRegistry &resources, const UIImage &image, Rect rect,
	f32 viewportHeight, f32 scale, Color color, u32 token);
void DrawUINinePatchRect(Renderer2D &renderer, ResourceRegistry &resources,
	const UINinePatchRect &image, Rect rect, f32 viewportHeight, f32 scale, Color color, u32 token);
} // namespace sowa
