#include "ui/ui_system.hpp"

#include <algorithm>
#include <cmath>
#include <numeric>

#include "ecs/components/camera2d.hpp"
#include "ecs/components/render_order.hpp"
#include "ecs/components/transform2d.hpp"
#include "ecs/world.hpp"
#include "resource/font.hpp"
#include "resource/image_texture.hpp"
#include "resource/resource_registry.hpp"
#include "ui/geometry.hpp"

namespace sowa {
namespace {
f32 positive(f32 value, f32 fallback = 0) {
	return std::isfinite(value) ? std::max(0.f, value) : fallback;
}

f32 bound(f32 value, f32 minimum, f32 maximum) {
	minimum = positive(minimum);
	return std::clamp(positive(value), minimum,
		maximum > 0 && std::isfinite(maximum) ? std::max(minimum, maximum) : 1e7f);
}

Color multiply(Color a, Color b) {
	return {a.r * b.r, a.g * b.g, a.b * b.b, a.a * b.a};
}
} // namespace

Rect UIIntersect(Rect a, Rect b) {
	const f32 x = std::max(a.x, b.x), y = std::max(a.y, b.y);
	return {x, y, std::max(0.f, std::min(a.Right(), b.Right()) - x),
		std::max(0.f, std::min(a.Top(), b.Top()) - y)};
}

f32 UIRangeRatio(f32 value, f32 minimum, f32 maximum) {
	if (!std::isfinite(value) || !std::isfinite(minimum) || !std::isfinite(maximum) ||
		maximum <= minimum) {
		return 0;
	}
	return std::clamp((value - minimum) / (maximum - minimum), 0.f, 1.f);
}

std::vector<std::string> UIItems(const std::string &text) {
	std::vector<std::string> items;
	if (text.empty()) {
		return items;
	}
	size_t start = 0;
	while (start <= text.size()) {
		const auto end = text.find('\n', start);
		items.push_back(text.substr(start, end == std::string::npos ? end : end - start));
		if (!items.back().empty() && items.back().back() == '\r') {
			items.back().pop_back();
		}
		if (end == std::string::npos) {
			break;
		}
		start = end + 1;
	}
	return items;
}

void UISystem::Init(World &world, ResourceRegistry &resources) {
	_resources = &resources;
	_canvases = world.CreateQuery<UICanvas>(VisibilityFilter::VisibleOnly);
}

void UISystem::Reset() {
	_focus = {};
	_hover = {};
	_capture = {};
	_modal = {};
	_lastModal = {};
	_modalFocus.clear();
	_dropdown = {};
	_inputEvents.clear();
	_subscriptions.clear();
	_nodes.clear();
	_indices.clear();
	_order.clear();
	_paintOrder.clear();
	_pointerConsumed = _keyboardConsumed = false;
}

UISystem::Node *UISystem::find(Entity e) {
	const auto it = _indices.find(e);
	return it == _indices.end() || !e.IsAlive() ? nullptr : &_nodes[it->second];
}

const UISystem::Node *UISystem::find(Entity e) const {
	const auto it = _indices.find(e);
	return it == _indices.end() || !e.IsAlive() ? nullptr : &_nodes[it->second];
}

const Font *UISystem::fontFor(const Node &node, ResourceID override) const {
	ResourceID id = override;
	if (!id) {
		id = node.style.font;
	}
	if (!id) {
		id = _defaultFont;
	}
	const auto font = _resources->Get<Font>(id);
	return font && font->HasGlyphs() ? font.get() : nullptr;
}

int UISystem::gather(Entity e, int parent, const Node &inherited) {
	if (!e.IsEnabled() || !e.IsVisibleInTree()) {
		return -1;
	}
	if (parent >= 0 && e.Has<UICanvas>()) {
		return -1;
	}
	if (const auto *popup = e.Get<UIPopup>(); popup && !popup->open) {
		return -1;
	}
	Node node = inherited;
	node.entity = e;
	node.parent = parent;
	node.children.clear();
	if (const auto *control = e.Get<UIControl>()) {
		node.enabled &= !control->disabled;
	}
	if (const auto *style = e.Get<UIStyle>()) {
		if (style->theme) {
			node.theme = style->theme;
		}
		if (!style->variant.empty()) {
			node.variant = style->variant;
		}
		node.tint = multiply(node.tint, style->modulate);
	}
	if (e.Has<UIPopup>()) {
		++node.overlay;
	}
	const auto theme = _resources->Get<UITheme>(node.theme);
	node.style = theme ? theme->Resolve(UIRole(e), node.variant) : UIAppearance{};
	const int index = static_cast<int>(_nodes.size());
	_nodes.push_back(node);
	_indices[e] = index;
	const auto children = e.GetChildren();
	const auto *tabs = e.Get<UITabs>();
	int page = 0;
	for (Entity child : children) {
		if (tabs && child.Has<UIRect>() && page++ != tabs->selected) {
			continue;
		}
		const int childIndex = gather(child, index, node);
		if (childIndex >= 0) {
			_nodes[index].children.push_back(childIndex);
		}
	}
	return index;
}

Vector2 UISystem::measure(int index, Vector2 available) {
	Node &node = _nodes[index];
	const auto *rect = node.entity.Get<UIRect>();
	const auto *layout = node.entity.Get<UILayout>();
	const f32 scale = node.scale;
	Vector2 size = rect ? rect->size * scale : available;
	if (rect && rect->anchorMax.x > rect->anchorMin.x) {
		size.x += available.x * (rect->anchorMax.x - rect->anchorMin.x);
	}
	if (rect && rect->anchorMax.y > rect->anchorMin.y) {
		size.y += available.y * (rect->anchorMax.y - rect->anchorMin.y);
	}
	Vector2 content{0, 0};
	Vector2 padding = layout ? layout->padding * scale : Vector2{};
	if (const auto *canvas = node.entity.Get<UICanvas>()) {
		padding += canvas->safePadding * scale;
	}
	const Vector2 inner{std::max(0.f, size.x - 2 * padding.x),
		std::max(0.f, size.y - 2 * padding.y)};
	std::string text;
	ResourceID font;
	f32 fontSize = node.style.fontSize * scale;
	bool wrap = false;
	f32 lineSpacing = 1;
	if (const auto *label = node.entity.Get<UILabel>()) {
		text = label->text;
		font = label->font;
		if (label->fontSize > 0) {
			fontSize = label->fontSize * scale;
		}
		wrap = label->wrap;
		lineSpacing = label->lineSpacing;
	} else if (const auto *button = node.entity.Get<UIButton>()) {
		text = button->text;
	} else if (const auto *toggle = node.entity.Get<UIToggle>()) {
		text = toggle->text;
	}
	if (!text.empty()) {
		if (const auto *f = fontFor(node, font)) {
			TextLayoutOptions options;
			options.fontSize = fontSize;
			options.lineSpacing = lineSpacing;
			options.wrapWidth = wrap ? std::max(1.f, inner.x) : 0;
			const auto block = LayoutTextBlock(*f, text, options);
			content = {block.bounds.w, block.bounds.h};
		} else {
			content = {fontSize * 0.55f * text.size(), fontSize};
		}
		if (!node.entity.Has<UILabel>()) {
			content += node.style.padding * scale * 2;
		}
	}
	if (const auto *image = node.entity.Get<UIImage>()) {
		if (const auto texture = _resources->Get<ImageTexture>(image->texture)) {
			content = texture->Size() * scale;
			if (image->regionSize.x > 0 && image->regionSize.y > 0) {
				content = image->regionSize * scale;
			}
		}
		if (image->mode == UIImageMode::NineSlice) {
			content = (image->borderMin + image->borderMax) * scale;
		}
	}
	if (const auto *image = node.entity.Get<UINinePatchRect>()) {
		content = {std::max(0.f, image->paddingLeft) + std::max(0.f, image->paddingRight),
			std::max(0.f, image->paddingTop) + std::max(0.f, image->paddingBottom)};
		content *= scale;
	}
	f32 rowWidth = 0, rowHeight = 0, flowHeight = 0;
	const Vector2 spacing = layout ? layout->spacing * scale : Vector2{};
	for (size_t childNumber = 0; childNumber < node.children.size(); ++childNumber) {
		const int child = node.children[childNumber];
		Vector2 childAvailable = inner;
		// Wrapped content is measured at its arranged cross-axis width.
		if (layout && layout->mode == UILayoutMode::Vertical &&
			layout->alignment == UIAlignment::Stretch) {
			if (auto *settings = _nodes[child].entity.Get<UIRect>();
				settings && settings->fitContentY) {
				const auto *l = _nodes[child].entity.Get<UILabel>();
				if (l && l->wrap) {
					childAvailable.x = inner.x - 2 * settings->margin.x * scale;
				}
			}
		}
		Vector2 desired = measure(child, childAvailable);
		const auto *childRect = _nodes[child].entity.Get<UIRect>();
		const Vector2 margin = childRect ? childRect->margin * scale : Vector2{};
		desired += margin * 2;
		const auto mode = layout ? layout->mode : UILayoutMode::Free;
		if (mode == UILayoutMode::Vertical) {
			content.x = std::max(content.x, desired.x);
			content.y += desired.y + (childNumber ? spacing.y : 0);
		} else if (mode == UILayoutMode::Horizontal) {
			content.x += desired.x + (childNumber ? spacing.x : 0);
			content.y = std::max(content.y, desired.y);
		} else if (mode == UILayoutMode::Grid) {
			const int columns = std::max(1, layout->columns);
			rowWidth = std::max(rowWidth, desired.x);
			rowHeight = std::max(rowHeight, desired.y);
			content = {rowWidth * columns + spacing.x * (columns - 1),
				rowHeight * std::ceil(float(node.children.size()) / columns) +
					spacing.y * (std::ceil(float(node.children.size()) / columns) - 1)};
		} else if (mode == UILayoutMode::Flow) {
			if (rowWidth > 0 && rowWidth + spacing.x + desired.x > inner.x) {
				flowHeight += rowHeight + spacing.y;
				rowWidth = 0;
				rowHeight = 0;
			}
			rowWidth += (rowWidth > 0 ? spacing.x : 0) + desired.x;
			rowHeight = std::max(rowHeight, desired.y);
			content = {std::max(content.x, rowWidth), flowHeight + rowHeight};
		} else {
			const Vector2 position = childRect ? childRect->position * scale : Vector2{};
			content.x = std::max(content.x, desired.x + position.x);
			content.y = std::max(content.y, desired.y + position.y);
		}
	}
	content += padding * 2;
	if (const auto *tabs = node.entity.Get<UITabs>()) {
		content.y += tabs->headerHeight * scale;
	}
	if (rect) {
		if (rect->fitContentX) {
			size.x = content.x;
		}
		if (rect->fitContentY) {
			size.y = content.y;
		}
		size.x = bound(size.x, rect->minSize.x * scale, rect->maxSize.x * scale);
		size.y = bound(size.y, rect->minSize.y * scale, rect->maxSize.y * scale);
		node.entity.Get<UIRect>()->measured = size / scale;
	}
	node.desired = size;
	return size;
}

Rect UISystem::anchored(const Node &node, Rect parent) const {
	Entity entity = node.entity;
	auto *settings = entity.Get<UIRect>();
	if (!settings) {
		return parent;
	}
	const f32 scale = node.scale;
	settings->anchorSpan = {(settings->anchorMax.x - settings->anchorMin.x) * parent.w / scale,
		(settings->anchorMax.y - settings->anchorMin.y) * parent.h / scale};
	const f32 w = bound((settings->anchorMax.x - settings->anchorMin.x) * parent.w +
							(settings->fitContentX ? node.desired.x : settings->size.x * scale),
		settings->minSize.x * scale, settings->maxSize.x * scale);
	const f32 h = bound((settings->anchorMax.y - settings->anchorMin.y) * parent.h +
							(settings->fitContentY ? node.desired.y : settings->size.y * scale),
		settings->minSize.y * scale, settings->maxSize.y * scale);
	return {parent.x + parent.w * settings->anchorMin.x + settings->position.x * scale -
				settings->pivot.x * w,
		parent.y + parent.h * settings->anchorMin.y + settings->position.y * scale -
			settings->pivot.y * h,
		w, h};
}

void UISystem::arrange(int index, Rect rect, Rect clip) {
	Node &node = _nodes[index];
	node.rect = rect;
	node.clip = clip;
	if (auto *list = node.entity.Get<UIList>()) {
		list->scroll = std::clamp(positive(list->scroll), 0.f,
			std::max(0.f, float(UIItems(list->items).size()) * std::max(1.f, list->rowHeight) -
							  rect.h / node.scale));
	}
	if (auto *dropdown = node.entity.Get<UIDropdown>()) {
		dropdown->scroll = std::clamp(positive(dropdown->scroll), 0.f,
			float(std::max(0,
				int(UIItems(dropdown->items).size()) - std::max(1, dropdown->maxVisibleRows))));
	}
	if (auto *settings = node.entity.Get<UIRect>()) {
		settings->resolved = rect;
		settings->viewportCorners =
			UIRectCorners(rect, node.canvas.Get<UICanvas>()->layoutToViewport);
		if (settings->clipContents) {
			clip = UIIntersect(clip, rect);
		}
	}
	const auto *layout = node.entity.Get<UILayout>();
	auto *scroll = node.entity.Get<UIScrollView>();
	const f32 scale = node.scale;
	Vector2 padding = layout ? layout->padding * scale : Vector2{};
	if (const auto *canvas = node.entity.Get<UICanvas>()) {
		padding += canvas->safePadding * scale;
	}
	const Vector2 spacing = layout ? layout->spacing * scale : Vector2{};
	Rect inner{rect.x + padding.x, rect.y + padding.y, std::max(0.f, rect.w - 2 * padding.x),
		std::max(0.f, rect.h - 2 * padding.y)};
	if (const auto *tabs = node.entity.Get<UITabs>()) {
		inner.y += tabs->headerHeight * scale;
		inner.h = std::max(0.f, inner.h - tabs->headerHeight * scale);
	}
	std::vector<Rect> placements;
	const auto mode = layout ? layout->mode : UILayoutMode::Free;
	f32 used = 0, grow = 0, maxHeight = 0;
	for (int child : node.children) {
		const auto *settings = _nodes[child].entity.Get<UIRect>();
		const auto margin = settings ? settings->margin * scale : Vector2{};
		if (mode == UILayoutMode::Horizontal) {
			used += _nodes[child].desired.x + 2 * margin.x;
		} else {
			used += _nodes[child].desired.y + 2 * margin.y;
		}
		grow += settings ? positive(settings->grow) : 0;
		maxHeight = std::max(maxHeight, _nodes[child].desired.y + 2 * margin.y);
	}
	if (!node.children.empty()) {
		used +=
			(node.children.size() - 1) * (mode == UILayoutMode::Horizontal ? spacing.x : spacing.y);
	}
	const f32 extra = std::max(0.f, (mode == UILayoutMode::Horizontal ? inner.w : inner.h) - used);
	f32 cursor = 0, flowY = 0, flowHeight = 0;
	Vector2 extent{inner.w, inner.h};
	for (size_t childNumber = 0; childNumber < node.children.size(); ++childNumber) {
		Node &child = _nodes[node.children[childNumber]];
		const auto *settings = child.entity.Get<UIRect>();
		const Vector2 margin = settings ? settings->margin * scale : Vector2{};
		Rect placement = anchored(child, inner);
		if (mode == UILayoutMode::Horizontal || mode == UILayoutMode::Vertical) {
			const bool horizontal = mode == UILayoutMode::Horizontal;
			f32 main = (horizontal ? child.desired.x : child.desired.y) +
					   (grow > 0 && settings ? extra * positive(settings->grow) / grow : 0);
			f32 cross = horizontal ? child.desired.y : child.desired.x;
			const f32 crossAvailable = std::max(0.f,
				(horizontal ? inner.h : inner.w) - 2 * (horizontal ? margin.y : margin.x));
			if (layout->alignment == UIAlignment::Stretch) {
				cross = crossAvailable;
			}
			if (settings) {
				main = bound(main, (horizontal ? settings->minSize.x : settings->minSize.y) * scale,
					(horizontal ? settings->maxSize.x : settings->maxSize.y) * scale);
			}
			if (settings) {
				cross =
					bound(cross, (horizontal ? settings->minSize.y : settings->minSize.x) * scale,
						(horizontal ? settings->maxSize.y : settings->maxSize.x) * scale);
			}
			f32 shift = layout->alignment == UIAlignment::Center ? (crossAvailable - cross) / 2
						: layout->alignment == UIAlignment::End	 ? crossAvailable - cross
																 : 0;
			placement =
				horizontal
					? Rect(inner.x + cursor + margin.x, inner.y + margin.y + shift, main, cross)
					: Rect(inner.x + margin.x + shift, inner.y + cursor + margin.y, cross, main);
			// Reflow a wrapped label after the container assigns its width.
			if (!horizontal && settings && settings->fitContentY) {
				if (const auto *l = child.entity.Get<UILabel>(); l && l->wrap) {
					if (const auto *font = fontFor(child, l->font)) {
						TextLayoutOptions o;
						o.fontSize = (l->fontSize > 0 ? l->fontSize : child.style.fontSize) * scale;
						o.wrapWidth = std::max(1.f, placement.w);
						o.lineSpacing = l->lineSpacing;
						placement.h = bound(LayoutTextBlock(*font, l->text, o).bounds.h,
							settings->minSize.y * scale, settings->maxSize.y * scale);
						main = placement.h;
					}
				}
			}
			cursor += main + 2 * (horizontal ? margin.x : margin.y) +
					  (horizontal ? spacing.x : spacing.y);
		} else if (mode == UILayoutMode::Grid) {
			const int columns = std::max(1, layout->columns);
			const f32 cell = std::max(0.f, (inner.w - (columns - 1) * spacing.x) / columns);
			placement = {inner.x + (childNumber % columns) * (cell + spacing.x) + margin.x,
				inner.y + (childNumber / columns) * (maxHeight + spacing.y) + margin.y,
				std::max(0.f, cell - 2 * margin.x), std::max(0.f, maxHeight - 2 * margin.y)};
		} else if (mode == UILayoutMode::Flow) {
			const f32 w = child.desired.x + 2 * margin.x, h = child.desired.y + 2 * margin.y;
			if (cursor > 0 && cursor + w > inner.w) {
				cursor = 0;
				flowY += flowHeight + spacing.y;
				flowHeight = 0;
			}
			placement = {inner.x + cursor + margin.x, inner.y + flowY + margin.y, child.desired.x,
				child.desired.y};
			cursor += w + spacing.x;
			flowHeight = std::max(flowHeight, h);
		} else if (mode == UILayoutMode::Overlay) {
			placement = {inner.x + margin.x, inner.y + margin.y,
				std::max(0.f, inner.w - 2 * margin.x), std::max(0.f, inner.h - 2 * margin.y)};
		}
		if (settings) {
			placement.w =
				bound(placement.w, settings->minSize.x * scale, settings->maxSize.x * scale);
			placement.h =
				bound(placement.h, settings->minSize.y * scale, settings->maxSize.y * scale);
		}
		if (settings && mode != UILayoutMode::Free) {
			child.entity.Get<UIRect>()->anchorSpan = {};
			placement.x += settings->position.x * scale;
			placement.y += settings->position.y * scale;
		}
		placements.push_back(placement);
		extent.x = std::max(extent.x, placement.Right() - inner.x);
		extent.y = std::max(extent.y, placement.Top() - inner.y);
	}
	Vector2 offset;
	if (scroll) {
		clip = UIIntersect(clip, rect);
		scroll->contentSize = (extent + padding * 2) / scale;
		scroll->offset.x = scroll->horizontal
							   ? std::clamp(positive(scroll->offset.x), 0.f,
									 std::max(0.f, scroll->contentSize.x - rect.w / scale))
							   : 0;
		scroll->offset.y = scroll->vertical
							   ? std::clamp(positive(scroll->offset.y), 0.f,
									 std::max(0.f, scroll->contentSize.y - rect.h / scale))
							   : 0;
		offset = scroll->offset * scale;
	}
	for (size_t childNumber = 0; childNumber < placements.size(); ++childNumber) {
		auto placement = placements[childNumber];
		placement.x -= offset.x;
		placement.y -= offset.y;
		arrange(node.children[childNumber], placement, clip);
	}
}

Vector2 UISystem::toLayout(const Node &node, Vector2 point) const {
	return UITransformPoint(node.canvas.Get<UICanvas>()->viewportToLayout, point);
}

void UISystem::Layout(Vector2 viewportSize) {
	Layout(viewportSize, CalculateBlankViewProjection(viewportSize));
}

void UISystem::Layout(Vector2 viewportSize, const glm::mat4 &worldViewProjection) {
	_viewport = {std::max(1.f, positive(viewportSize.x, 1)),
		std::max(1.f, positive(viewportSize.y, 1))};
	_nodes.clear();
	_indices.clear();
	_order.clear();
	_paintOrder.clear();
	_modal = {};
	if (!_canvases.IsValid()) {
		return;
	}

	std::vector<Entity> canvases;
	_canvases.Each([&](Entity entity, UICanvas &) {
		if (entity.IsVisibleInTree()) {
			canvases.push_back(entity);
		}
	});
	std::stable_sort(canvases.begin(), canvases.end(), [](Entity a, Entity b) {
		const auto &first = *a.Get<UICanvas>();
		const auto &second = *b.Get<UICanvas>();
		if (first.space != second.space) {
			return first.space == UISpace::World;
		}
		if (first.sortOrder != second.sortOrder) {
			return first.sortOrder < second.sortOrder;
		}
		return a.GetPath() < b.GetPath();
	});
	for (size_t index = 0; index < canvases.size(); ++index) {
		Entity entity = canvases[index];
		auto &canvas = *entity.Get<UICanvas>();
		if (!UpdateUICanvasMapping(entity, _viewport, worldViewProjection)) {
			continue;
		}

		Node base;
		base.canvas = entity;
		base.canvasOrder = static_cast<int>(index);
		base.theme = canvas.theme;
		base.scale = canvas.unitScale;
		const int root = gather(entity, -1, base);
		if (root < 0) {
			continue;
		}

		Vector2 available = canvas.referenceSize;
		if (canvas.space == UISpace::Screen) {
			available = _viewport;
		}
		measure(root, available);
		canvas.bounds = anchored(_nodes[root], {0, 0, available.x, available.y});
		arrange(root, canvas.bounds, canvas.bounds);
	}
	buildPaintOrder();
	std::erase_if(_subscriptions,
		[](const Subscription &subscription) { return !subscription.entity.IsAlive(); });
}

void UISystem::buildPaintOrder() {
	_order.resize(_nodes.size());
	std::iota(_order.begin(), _order.end(), 0);
	std::stable_sort(_order.begin(), _order.end(), [&](int a, int b) {
		const auto &x = _nodes[a], &y = _nodes[b];
		if (x.canvasOrder != y.canvasOrder) {
			return x.canvasOrder < y.canvasOrder;
		}
		if (x.overlay != y.overlay) {
			return x.overlay < y.overlay;
		}
		return ComputeEffectiveZIndex(x.entity) < ComputeEffectiveZIndex(y.entity);
	});
	std::vector<int> last(_nodes.size(), -1);
	for (size_t at = 0; at < _order.size(); ++at) {
		const int index = _order[at];
		for (int parent = index; parent >= 0; parent = _nodes[parent].parent) {
			if (_nodes[parent].overlay == _nodes[index].overlay) {
				last[parent] = at;
			}
		}
	}
	std::vector<std::vector<int>> bars(_order.size());
	for (auto it = _order.rbegin(); it != _order.rend(); ++it) {
		if (_nodes[*it].entity.Has<UIScrollView>() && last[*it] >= 0) {
			bars[last[*it]].push_back(*it);
		}
	}
	for (size_t at = 0; at < _order.size(); ++at) {
		_paintOrder.emplace_back(_order[at], false);
		for (int index : bars[at]) {
			_paintOrder.emplace_back(index, true);
		}
	}
	for (int i : _order) {
		if (const auto *popup = _nodes[i].entity.Get<UIPopup>(); popup && popup->modal) {
			_modal = _nodes[i].entity;
		}
	}
}
} // namespace sowa
