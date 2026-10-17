#include "ui/ui_system.hpp"

#include <algorithm>
#include <cmath>

#include "ecs/entity_pick_map.hpp"
#include "render/renderer2d.hpp"
#include "resource/font.hpp"
#include "ui/drawing.hpp"

namespace sowa {
namespace {
Color multiply(Color a, Color b) {
	return {a.r * b.r, a.g * b.g, a.b * b.b, a.a * b.a};
}

Rect inset(Rect r, f32 x, f32 y) {
	return {r.x + x, r.y + y, std::max(0.f, r.w - x * 2), std::max(0.f, r.h - y * 2)};
}
} // namespace

UIPainter UISystem::painter(const Node &node, Renderer2D &renderer, u32 token) const {
	Color textColor = node.style.text;
	if (!node.enabled) {
		textColor.a *= .5f;
	}
	return {.renderer = renderer,
		.resources = *_resources,
		.style = node.style,
		.defaultFont = fontFor(node),
		.viewportHeight = _viewport.y,
		.scale = node.scale,
		.tint = node.tint,
		.textColor = textColor,
		.token = token};
}

void UISystem::drawNode(const Node &n, Renderer2D &renderer, u32 token) {
	Entity e = n.entity;
	const Rect rect = n.rect;
	const auto &style = n.style;
	const f32 s = n.scale;
	const auto *control = e.Get<UIControl>();
	auto paint = painter(n, renderer, token);
	const Color textColor = paint.textColor;
	const Rect content = inset(rect, style.padding.x * s, style.padding.y * s);
	const bool button = e.Has<UIButton>() || e.Has<UIToggle>() || e.Has<UIDropdown>();
	const bool surface = button || e.Has<UIPanel>() || e.Has<UIPopup>() || e.Has<UISlider>() ||
						 e.Has<UIProgressBar>() || e.Has<UITextInput>() || e.Has<UIList>() ||
						 e.Has<UITabs>() || e.Has<UIScrollView>() || e.Has<UIScrollbar>();
	Color background = style.background;
	if (!n.enabled) {
		background = style.disabled;
	} else if (button && control) {
		if (control->pressed) {
			background = style.pressed;
		} else if (control->hovered) {
			background = style.hovered;
		}
	}
	if (surface) {
		if (const auto *panel = e.Get<UIPanel>()) {
			background = multiply(background, panel->modulate);
		}
		paint.Surface(rect, background);
		if (style.borderWidth > 0) {
			const Color border = control && control->focused ? style.accent : style.border;
			paint.Outline(rect, border, std::min({style.borderWidth * s, rect.w / 2, rect.h / 2}));
		}
	}
	if (const auto *image = e.Get<UINinePatchRect>()) {
		paint.NinePatch(*image, rect);
	}
	if (const auto *image = e.Get<UIImage>()) {
		paint.Image(*image, rect, image->modulate);
	}
	if (const auto *label = e.Get<UILabel>()) {
		paint.Text(label->text, rect,
			{.alignment = label->alignment,
				.color = multiply(label->modulate, textColor),
				.wrap = label->wrap,
				.overflow = label->overflow,
				.font = label->font,
				.fontSize = label->fontSize,
				.vertical = label->verticalAlignment,
				.lineSpacing = label->lineSpacing});
	}
	if (const auto *button = e.Get<UIButton>()) {
		paint.Text(button->text, content, {.overflow = UIOverflow::Ellipsis});
	}
	if (const auto *toggle = e.Get<UIToggle>()) {
		const f32 size = std::min(24.f * s, content.h);
		Rect box{content.x, content.y + (content.h - size) / 2, size, size};
		paint.Quad(box, toggle->checked ? style.accent : style.pressed);
		if (toggle->checked) {
			const f32 w = std::max(1.f, 2 * s);
			renderer.DrawLine({box.x + size * .2f, _viewport.y - box.y - size * .5f},
				{box.x + size * .43f, _viewport.y - box.y - size * .73f}, textColor, w);
			renderer.DrawLine({box.x + size * .43f, _viewport.y - box.y - size * .73f},
				{box.x + size * .82f, _viewport.y - box.y - size * .25f}, textColor, w);
		}
		paint.Text(toggle->text,
			{box.Right() + 10 * s, content.y, std::max(0.f, content.w - size - 10 * s), content.h},
			{.alignment = TextHorizontalAlignment::Left, .overflow = UIOverflow::Ellipsis});
	}
	if (const auto *progress = e.Get<UIProgressBar>()) {
		const f32 t = UIRangeRatio(progress->value, progress->minimum, progress->maximum);
		Rect fill = inset(rect, 2 * s, 2 * s);
		if (progress->vertical) {
			fill.y += fill.h * (1 - t);
			fill.h *= t;
		} else {
			fill.w *= t;
		}
		paint.Quad(fill, style.accent);
		if (progress->showValue) {
			paint.Text(std::to_string(int(std::round(t * 100))) + "%", content);
		}
	}
	if (const auto *slider = e.Get<UISlider>()) {
		const f32 t = UIRangeRatio(slider->value, slider->minimum, slider->maximum);
		Rect track = slider->vertical ? Rect(rect.x + rect.w / 2 - 2 * s, rect.y, 4 * s, rect.h)
									  : Rect(rect.x, rect.y + rect.h / 2 - 2 * s, rect.w, 4 * s);
		paint.Quad(track, style.border);
		const f32 diameter = std::min(18.f * s, slider->vertical ? rect.w : rect.h);
		Rect thumb = slider->vertical
						 ? Rect(rect.x + (rect.w - diameter) / 2,
							   rect.y + (1 - t) * (rect.h - diameter), diameter, diameter)
						 : Rect(rect.x + t * (rect.w - diameter), rect.y + (rect.h - diameter) / 2,
							   diameter, diameter);
		paint.Quad(thumb, style.accent);
	}
	if (auto *input = e.Get<UITextInput>()) {
		drawTextInput(n, paint, *input);
	}
	if (const auto *list = e.Get<UIList>()) {
		const auto items = UIItems(list->items);
		const f32 row = std::max(1.f, list->rowHeight) * s;
		const f32 scroll =
			std::clamp(list->scroll * s, 0.f, std::max(0.f, items.size() * row - rect.h));
		const int first = std::max(0, int(scroll / row)),
				  last = std::min(int(items.size()), int(std::ceil((scroll + rect.h) / row)));
		renderer.PushClip({rect.x, _viewport.y - rect.y - rect.h, rect.w, rect.h});
		for (int i = first; i < last; ++i) {
			Rect r{rect.x, rect.y + i * row - scroll, rect.w, row};
			if (i == list->selected) {
				paint.Quad(r, style.accent);
			}
			paint.Text(items[i], inset(r, style.padding.x * s, 0),
				{.alignment = TextHorizontalAlignment::Left, .overflow = UIOverflow::Ellipsis});
		}
		renderer.PopClip();
	}
	if (const auto *dropdown = e.Get<UIDropdown>()) {
		const auto items = UIItems(dropdown->items);
		const std::string label = dropdown->selected >= 0 && dropdown->selected < int(items.size())
									  ? items[dropdown->selected]
									  : "Select...";
		paint.Text(label, {content.x, content.y, std::max(0.f, content.w - 24 * s), content.h},
			{.alignment = TextHorizontalAlignment::Left, .overflow = UIOverflow::Ellipsis});
		paint.Text(dropdown->open ? "-" : "v",
			{content.Right() - 18 * s, content.y, 18 * s, content.h});
	}
	if (const auto *tabs = e.Get<UITabs>()) {
		const auto titles = UIItems(tabs->titles);
		const f32 width = titles.empty() ? 0 : rect.w / titles.size();
		for (size_t i = 0; i < titles.size(); ++i) {
			Rect r{rect.x + i * width, rect.y, width, tabs->headerHeight * s};
			paint.Quad(r, int(i) == tabs->selected ? style.hovered : style.background);
			if (int(i) == tabs->selected) {
				paint.Quad({r.x, r.Top() - 3 * s, r.w, 3 * s}, style.accent);
			}
			paint.Text(titles[i], inset(r, 6 * s, 0), {.overflow = UIOverflow::Ellipsis});
		}
	}
	if (const auto *bar = e.Get<UIScrollbar>()) {
		if (const auto *target = find(bar->target)) {
			if (const auto *scroll = bar->target.Get<UIScrollView>()) {
				const f32 view = bar->vertical ? target->rect.h : target->rect.w,
						  full = (bar->vertical ? scroll->contentSize.y : scroll->contentSize.x) *
								 target->scale;
				const f32 track = bar->vertical ? rect.h : rect.w,
						  thumb = std::min(track,
							  std::max(20 * s, track * view / std::max(view, full)));
				const f32 offset =
					(bar->vertical ? scroll->offset.y : scroll->offset.x) * target->scale;
				const f32 at = full > view ? offset / (full - view) * (track - thumb) : 0;
				paint.Quad(bar->vertical ? Rect(rect.x, rect.y + at, rect.w, thumb)
										 : Rect(rect.x + at, rect.y, thumb, rect.h),
					style.accent);
			}
		}
	}
}

void UISystem::drawTextInput(const Node &n, UIPainter &paint, UITextInput &input) {
	const auto &style = n.style;
	const f32 s = n.scale;
	const Color textColor = paint.textColor;
	const Rect content = inset(n.rect, style.padding.x * s, style.padding.y * s);
	const auto *control = n.entity.Get<UIControl>();

	input.caret = std::min<u32>(input.caret, input.text.size());
	input.selectionAnchor = std::min<u32>(input.selectionAnchor, input.text.size());
	const auto *font = fontFor(n);
	const f32 size = style.fontSize * s;
	const f32 ratio = font ? size / (font->Ascent() - font->Descent()) : 1;
	const f32 line = font ? font->LineHeight() * ratio : size;
	const auto a = std::min(input.caret, input.selectionAnchor),
			   b = std::max(input.caret, input.selectionAnchor);
	f32 x = 0, y = 0, caretX = 0, caretY = 0;
	std::string display;

	struct Selection {
		f32 x, y, w;
	};

	std::vector<Selection> selections;
	for (size_t p = 0; p <= input.text.size(); ++p) {
		if (p == input.caret) {
			caretX = x;
			caretY = y;
		}
		if (p == input.text.size()) {
			break;
		}
		const u8 c = input.text[p];
		if ((c & 0xc0) == 0x80) {
			continue;
		}
		if (c == '\n') {
			display += '\n';
			if (p >= a && p < b) {
				selections.push_back({x, y, size * .3f});
			}
			x = 0;
			y += line;
			continue;
		}
		const char visible = input.password ? '*' : c < 128 ? char(c) : '?';
		display += visible;
		const auto *glyph = font ? font->FindGlyph(static_cast<u8>(visible)) : nullptr;
		const f32 advance = glyph ? glyph->advance * ratio : size * .55f;
		if (p >= a && p < b) {
			selections.push_back({x, y, advance});
		}
		x += advance;
	}
	if (control && control->focused) {
		if (caretX < input.scroll.x) {
			input.scroll.x = caretX;
		}
		if (caretX > input.scroll.x + content.w - 2 * s) {
			input.scroll.x = std::max(0.f, caretX - content.w + 2 * s);
		}
		if (caretY < input.scroll.y) {
			input.scroll.y = caretY;
		}
		if (caretY + line > input.scroll.y + content.h) {
			input.scroll.y = std::max(0.f, caretY + line - content.h);
		}
	}
	paint.Clip(content);
	for (const auto &selection : selections) {
		paint.Quad({content.x + selection.x - input.scroll.x,
					   content.y + selection.y - input.scroll.y, selection.w, line},
			Color(style.accent.r, style.accent.g, style.accent.b, .45f));
	}
	Rect textRect{content.x - input.scroll.x, content.y - input.scroll.y,
		std::max(content.w, x + 2 * s), std::max(content.h, y + line)};
	if (input.text.empty()) {
		paint.Text(input.placeholder, content,
			{.alignment = TextHorizontalAlignment::Left,
				.color = Color(textColor.r, textColor.g, textColor.b, textColor.a * .5f),
				.vertical = UIAlignment::Start});
	} else {
		paint.Text(display, textRect,
			{.alignment = TextHorizontalAlignment::Left,
				.overflow = UIOverflow::Visible,
				.vertical = UIAlignment::Start});
	}
	if (control && control->focused) {
		paint.Quad({content.x + caretX - input.scroll.x, content.y + caretY - input.scroll.y,
					   1.5f * s, line},
			style.accent);
	}
	paint.PopClip();
}

void UISystem::Draw(Renderer2D &renderer, EntityPickMap &picks) {
	Entity activeCanvas;
	auto beginCanvas = [&](const Node &node) {
		if (activeCanvas == node.canvas) {
			return;
		}
		if (activeCanvas.IsAlive()) {
			renderer.End();
		}
		activeCanvas = node.canvas;
		renderer.SetViewProjection(activeCanvas.Get<UICanvas>()->projection);
		renderer.Begin();
		renderer.SetAlphaCutoff(.001f);
	};
	for (const auto &[index, scrollbars] : _paintOrder) {
		const auto &n = _nodes[index];
		if (!n.entity.IsAlive()) {
			continue;
		}
		beginCanvas(n);
		auto paint = painter(n, renderer, picks.TokenFor(n.entity));
		if (!scrollbars) {
			if (const auto *popup = n.entity.Get<UIPopup>(); popup && popup->modal) {
				paint.Quad(n.canvas.Get<UICanvas>()->bounds, Color(0, 0, 0, .5f));
			}
			renderer.PushClip({n.clip.x, _viewport.y - n.clip.y - n.clip.h, n.clip.w, n.clip.h});
			drawNode(n, renderer, picks.TokenFor(n.entity));
			renderer.PopClip();
			continue;
		}
		const auto *scroll = n.entity.Get<UIScrollView>();
		if (!scroll || !scroll->showScrollbars) {
			continue;
		}
		renderer.PushClip({n.clip.x, _viewport.y - n.clip.y - n.clip.h, n.clip.w, n.clip.h});
		for (bool vertical : {false, true}) {
			if (!(vertical ? scroll->vertical : scroll->horizontal)) {
				continue;
			}
			const f32 view = vertical ? n.rect.h : n.rect.w,
					  full = (vertical ? scroll->contentSize.y : scroll->contentSize.x) * n.scale;
			if (full <= view) {
				continue;
			}
			const f32 thumb = std::min(view, std::max(20 * n.scale, view * view / full));
			const f32 at = (vertical ? scroll->offset.y : scroll->offset.x) * n.scale /
						   (full - view) * (view - thumb);
			const Rect r =
				vertical ? Rect(n.rect.Right() - 10 * n.scale, n.rect.y + at, 8 * n.scale, thumb)
						 : Rect(n.rect.x + at, n.rect.Top() - 10 * n.scale, thumb, 8 * n.scale);
			renderer.DrawQuad(r.x, _viewport.y - r.y - r.h, r.w, r.h, n.style.accent, 0,
				picks.TokenFor(n.entity));
		}
		renderer.PopClip();
	}
	if (const auto *n = find(_dropdown); n && eligible(*n)) {
		if (const auto *d = _dropdown.Get<UIDropdown>(); d && d->open) {
			beginCanvas(*n);
			auto paint = painter(*n, renderer, picks.TokenFor(n->entity));
			const Rect popup = dropdownRect(*n);
			const auto items = UIItems(d->items);
			const f32 row = std::max(20.f, n->style.fontSize + n->style.padding.y * 2) * n->scale;
			renderer.PushClip({popup.x, _viewport.y - popup.y - popup.h, popup.w, popup.h});
			paint.Surface(popup, n->style.background);
			const int first = std::max(0, int(d->scroll)),
					  last = std::min(int(items.size()), first + std::max(1, d->maxVisibleRows));
			for (int i = first; i < last; ++i) {
				Rect r{popup.x, popup.y + (i - first) * row, popup.w, row};
				if (i == d->selected) {
					paint.Surface(r, n->style.hovered);
				}
				paint.Text(items[i], inset(r, n->style.padding.x * n->scale, 0),
					{.alignment = TextHorizontalAlignment::Left, .overflow = UIOverflow::Ellipsis});
			}
			renderer.PopClip();
		}
	}
	if (const auto *n = find(_hover); n && eligible(*n)) {
		if (const auto *tip = n->entity.Get<UITooltip>();
			tip && _hoverTime >= tip->delay && !_capture.IsAlive()) {
			beginCanvas(*n);
			auto paint = painter(*n, renderer, 0);
			const Rect bounds = n->canvas.Get<UICanvas>()->bounds;
			const Vector2 point = toLayout(*n, _pointer);
			const f32 width = std::min(320.f * n->scale, bounds.w);
			const f32 height = std::min(80.f * n->scale, bounds.h);
			const Rect r{std::clamp(point.x + 12 * n->scale, bounds.x, bounds.Right() - width),
				std::clamp(point.y + 20 * n->scale, bounds.y, bounds.Top() - height), width,
				height};
			paint.Surface(r, n->style.background);
			paint.Text(tip->text, inset(r, 10 * n->scale, 8 * n->scale),
				{.alignment = TextHorizontalAlignment::Left,
					.wrap = true,
					.overflow = UIOverflow::Ellipsis});
		}
	}
	if (activeCanvas.IsAlive()) {
		renderer.End();
	}
}
} // namespace sowa
