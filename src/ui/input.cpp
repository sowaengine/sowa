#include "ui/ui_system.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

#include "resource/font.hpp"
#include "ui/geometry.hpp"

namespace sowa {
namespace {
bool contains(Rect r, Vector2 p) {
	return r.w > 0 && r.h > 0 && p.x >= r.x && p.y >= r.y && p.x < r.Right() && p.y < r.Top();
}

} // namespace

bool UISystem::eligible(const Node &n) const {
	if (!n.enabled || !n.entity.IsAlive()) {
		return false;
	}
	for (Entity e = n.entity; e.IsAlive(); e = e.GetParent()) {
		if (!e.IsEnabled() || !e.IsVisibleInTree()) {
			return false;
		}
		if (const auto *c = e.Get<UIControl>(); c && c->disabled) {
			return false;
		}
		if (const auto *p = e.Get<UIPopup>(); p && !p->open) {
			return false;
		}
	}
	return !_modal.IsAlive() || n.entity == _modal || n.entity.IsDescendantOf(_modal);
}

Rect UISystem::dropdownRect(const Node &node) const {
	const auto *dropdown = node.entity.Get<UIDropdown>();
	if (!dropdown) {
		return {};
	}
	const Rect bounds = node.canvas.Get<UICanvas>()->bounds;
	const f32 rowHeight =
		std::max(20.f, node.style.fontSize + node.style.padding.y * 2) * node.scale;
	const int rows =
		std::min<int>(UIItems(dropdown->items).size(), std::max(1, dropdown->maxVisibleRows));
	const f32 height = std::min(bounds.h, rowHeight * rows);
	const f32 width = std::min(bounds.w, node.rect.w);
	const f32 left = std::clamp(node.rect.x, bounds.x, bounds.Right() - width);
	f32 top = node.rect.Top();
	if (top + height > bounds.Top()) {
		top = std::max(bounds.y, node.rect.y - height);
	}
	return {left, top, width, height};
}

int UISystem::scrollbarPart(const Node &n, Vector2 p) const {
	const auto *s = n.entity.Get<UIScrollView>();
	if (!s || !s->showScrollbars || !contains(n.rect, p) || !contains(n.clip, p)) {
		return 0;
	}
	if (s->vertical && s->contentSize.y > n.rect.h / n.scale &&
		p.x > n.rect.Right() - 12 * n.scale) {
		return 2;
	}
	if (s->horizontal && s->contentSize.x > n.rect.w / n.scale &&
		p.y > n.rect.Top() - 12 * n.scale) {
		return 1;
	}
	return 0;
}

Entity UISystem::HitTest(Vector2 p) const {
	if (const auto *n = find(_dropdown); n && eligible(*n)) {
		const auto *d = n->entity.Get<UIDropdown>();
		if (d && d->open && contains(dropdownRect(*n), toLayout(*n, p))) {
			return n->entity;
		}
	}
	for (auto it = _paintOrder.rbegin(); it != _paintOrder.rend(); ++it) {
		const auto &n = _nodes[it->node];
		if (it->scrollbars) {
			if (eligible(n) && scrollbarPart(n, toLayout(n, p))) {
				return n.entity;
			}
			continue;
		}
		const auto *control = n.entity.Get<UIControl>();
		if (!eligible(n) || !contains(n.rect, toLayout(n, p)) ||
			!contains(n.clip, toLayout(n, p))) {
			continue;
		}
		if (control && control->blockPointer) {
			return n.entity;
		}
		if (n.entity.Has<UIPopup>()) {
			return n.entity;
		}
	}
	return {};
}

void UISystem::pointerButton(const InputEvent &event, Entity &hit) {
	if (event.button.action == KeyAction::Press) {
		if (const auto *n = find(_dropdown);
			n && contains(dropdownRect(*n), toLayout(*n, _pointer))) {
			auto *d = _dropdown.Get<UIDropdown>();
			const auto popup = dropdownRect(*n);
			const f32 row = std::max(20.f, n->style.fontSize + n->style.padding.y * 2) * n->scale;
			Entity selected = _dropdown;
			const int item = int((toLayout(*n, _pointer).y - popup.y) / row + d->scroll);
			d->open = false;
			_dropdown = {};
			changeSelection(selected, item);
			hit = {};
			return;
		}
		if (_dropdown.IsAlive() && hit != _dropdown) {
			if (auto *d = _dropdown.Get<UIDropdown>()) {
				d->open = false;
			}
			_dropdown = {};
		}
		for (auto it = _order.rbegin(); it != _order.rend(); ++it) {
			const auto &n = _nodes[*it];
			const auto *p = n.entity.Get<UIPopup>();
			if (p && p->open && !contains(n.rect, toLayout(n, _pointer)) && p->dismissOutside) {
				_pointerConsumed = true;
				Open(n.entity, false);
				hit = {};
				break;
			}
		}
		if (hit.IsAlive()) {
			Focus(hit);
			if (!hit.IsAlive()) {
				return;
			}
			_capture = hit;
			_dragPart = 0;
			_dragStart = toLayout(*find(hit), _pointer);
			const auto *n = find(hit);
			_dragPart = scrollbarPart(*n, _dragStart);
			if (auto *scroll = hit.Get<UIScrollView>()) {
				_dragOffset = scroll->offset;
			}
			dragValue(hit, _pointer);
			if (hit.IsAlive()) {
				if (auto *t = hit.Get<UITextInput>()) {
					if (!(event.button.mods & 1)) {
						t->selectionAnchor = t->caret;
					}
				}
			}
			if (hit.IsAlive()) {
				emit(hit, "pressed");
			}
		} else if (!_modal.IsAlive()) {
			Focus({});
		}
	} else if (event.button.action == KeyAction::Release) {
		Entity captured = _capture;
		_capture = {};
		if (captured.IsAlive()) {
			emit(captured, "released");
		}
		if (captured.IsAlive() && captured == hit && find(captured) && eligible(*find(captured))) {
			const auto *n = find(captured);
			if (auto *l = captured.Get<UIList>(); l && n) {
				changeSelection(captured, int((toLayout(*n, _pointer).y - n->rect.y) /
												  (std::max(1.f, l->rowHeight) * n->scale) +
											  l->scroll / std::max(1.f, l->rowHeight)));
			} else if (auto *t = captured.Get<UITabs>();
				t && n && toLayout(*n, _pointer).y < n->rect.y + t->headerHeight * n->scale) {
				changeSelection(captured,
					int((toLayout(*n, _pointer).x - n->rect.x) / std::max(1.f, n->rect.w) *
						UIItems(t->titles).size()));
			} else if (captured.Has<UIButton>() || captured.Has<UIToggle>() ||
					   captured.Has<UIDropdown>()) {
				activate(captured);
			}
		}
	}
}

void UISystem::scrollInput(const InputEvent &event, Entity hit) {
	if (hit.IsAlive() && emit(hit, "wheel", event.scroll.yOffset, {},
							 {event.scroll.xOffset, event.scroll.yOffset})) {
		return;
	}
	for (Entity e = hit; e.IsAlive(); e = e.GetParent()) {
		const auto *n = find(e);
		if (!n) {
			continue;
		}
		if (auto *d = e.Get<UIDropdown>(); d && d->open) {
			d->scroll = std::clamp(d->scroll - event.scroll.yOffset, 0.f,
				float(std::max(0, int(UIItems(d->items).size()) - std::max(1, d->maxVisibleRows))));
			break;
		}
		if (auto *l = e.Get<UIList>()) {
			l->scroll = std::clamp(l->scroll - event.scroll.yOffset * l->rowHeight, 0.f,
				std::max(0.f,
					float(UIItems(l->items).size()) * l->rowHeight - n->rect.h / n->scale));
			break;
		}
		if (auto *s = e.Get<UIScrollView>()) {
			const Vector2 before = s->offset;
			ScrollTo(e,
				before - Vector2(event.scroll.xOffset, event.scroll.yOffset) * s->wheelSpeed);
			if (s->offset != before) {
				emit(e, "scrolled");
				break;
			}
		}
	}
}

void UISystem::HandleInput(Input &input, f32 dt, bool active) {
	_pointerConsumed = _keyboardConsumed = false;
	input.SetUICapture(false, false);
	if (!active) {
		_inputEvents.clear();
		Focus({});
		_capture = {};
		_hover = {};
		for (auto &n : _nodes) {
			if (n.entity.IsAlive()) {
				if (auto *c = n.entity.Get<UIControl>()) {
					c->hovered = c->pressed = c->focused = false;
				}
			}
		}
		return;
	}
	if (_lastModal != _modal) {
		const auto found = std::find_if(_modalFocus.begin(), _modalFocus.end(),
			[&](const auto &entry) { return entry.first == _modal; });
		if (_modal.IsAlive() && found == _modalFocus.end()) {
			_modalFocus.emplace_back(_modal, _focus);
			Focus({});
			moveFocus(false);
		} else {
			Entity restore;
			while (!_modalFocus.empty() && _modalFocus.back().first != _modal) {
				restore = _modalFocus.back().second;
				_modalFocus.pop_back();
			}
			if (!Focus(restore) && _modal.IsAlive()) {
				moveFocus(false);
			}
		}
		_lastModal = _modal;
	}
	if (_focus.IsAlive()) {
		const auto *n = find(_focus);
		if (!n || !eligible(*n)) {
			Focus({});
		}
	}
	if (_capture.IsAlive()) {
		const auto *n = find(_capture);
		if (!n || !eligible(*n)) {
			_capture = {};
		}
	}
	const Vector2 mapped = input.GetMousePosition();
	_pointer = {mapped.x, _viewport.y - mapped.y};
	Entity hit = input.IsMouseOverViewport() ? HitTest(_pointer) : Entity{};
	if (hit != _hover) {
		if (_hover.IsAlive()) {
			emit(_hover, "pointer_exit");
		}
		_hover = hit;
		_hoverTime = 0;
		if (hit.IsAlive()) {
			emit(hit, "pointer_enter");
		}
	} else {
		_hoverTime += std::max(0.f, dt);
	}
	std::vector<InputEvent> events;
	events.swap(_inputEvents);
	// Direct polling remains usable for headless callers that do not wire Events().
	if (events.empty()) {
		if (input.IsKeyJustPressed(Key::MouseLeft)) {
			InputEvent e;
			e.type = InputEventType::MouseButton;
			e.button.button = Key::MouseLeft;
			e.button.action = KeyAction::Press;
			e.windowPosition = input.GetWindowMousePosition();
			events.push_back(e);
		}
		if (input.IsKeyJustReleased(Key::MouseLeft)) {
			InputEvent e;
			e.type = InputEventType::MouseButton;
			e.button.button = Key::MouseLeft;
			e.button.action = KeyAction::Release;
			e.windowPosition = input.GetWindowMousePosition();
			events.push_back(e);
		}
		const auto delta = input.ScrollDelta();
		if (delta.x != 0 || delta.y != 0) {
			InputEvent e;
			e.type = InputEventType::Scroll;
			e.scroll = {delta.x, delta.y};
			e.windowPosition = input.GetWindowMousePosition();
			events.push_back(e);
		}
	}
	for (const auto &event : events) {
		if (_focus.IsAlive() && (!find(_focus) || !eligible(*find(_focus)))) {
			Focus({});
		}
		if (_capture.IsAlive() && (!find(_capture) || !eligible(*find(_capture)))) {
			_capture = {};
		}
		if (event.type == InputEventType::MouseMove || event.type == InputEventType::MouseButton ||
			event.type == InputEventType::Scroll) {
			const auto point = input.MapWindowPosition(event.windowPosition);
			_pointer = {point.x, _viewport.y - point.y};
			hit = (_pointer.x >= 0 && _pointer.y >= 0 && _pointer.x < _viewport.x &&
					  _pointer.y < _viewport.y)
					  ? HitTest(_pointer)
					  : Entity{};
		}
		if (event.type == InputEventType::MouseMove || event.type == InputEventType::MouseButton ||
			event.type == InputEventType::Scroll) {
			_pointerConsumed |= hit.IsAlive() || _capture.IsAlive() || _modal.IsAlive();
		} else {
			_keyboardConsumed |=
				_focus.IsAlive() || _modal.IsAlive() ||
				(event.type == InputEventType::Key && event.key.key == Key::Tab && !_nodes.empty());
		}
		if (event.type == InputEventType::MouseMove) {
			if (hit != _hover) {
				if (_hover.IsAlive()) {
					emit(_hover, "pointer_exit");
				}
				_hover = hit;
				_hoverTime = 0;
				if (hit.IsAlive()) {
					emit(hit, "pointer_enter");
				}
			}
			if (hit.IsAlive()) {
				emit(hit, "pointer_move");
			}
			if (_capture.IsAlive()) {
				dragValue(_capture, _pointer);
				emit(_capture, "drag");
			}
		} else if (event.type == InputEventType::MouseButton &&
				   event.button.button == Key::MouseLeft) {
			pointerButton(event, hit);
		} else if (event.type == InputEventType::Scroll) {
			scrollInput(event, hit);
		} else if (event.type == InputEventType::Key || event.type == InputEventType::Char) {
			keyInput(event);
		}
	}
	if (_capture.IsAlive() && input.IsKeyDown(Key::MouseLeft)) {
		dragValue(_capture, _pointer);
	}
	for (auto &n : _nodes) {
		if (n.entity.IsAlive()) {
			if (auto *c = n.entity.Get<UIControl>()) {
				c->hovered = n.entity == _hover;
				c->pressed = n.entity == _capture;
				c->focused = n.entity == _focus;
			}
		}
	}
	_pointerConsumed |= hit.IsAlive() || _capture.IsAlive() || _modal.IsAlive();
	_keyboardConsumed |= _focus.IsAlive() || _modal.IsAlive();
	input.SetUICapture(_pointerConsumed, _keyboardConsumed);
}
} // namespace sowa
