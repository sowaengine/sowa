#include "ui/geometry.hpp"
#include "ui/ui_system.hpp"
#include <algorithm>
#include <cmath>
#include <limits>

namespace sowa {
u64 UISystem::On(Entity e, std::string type, EventHandler handler) {
	if (!e.IsAlive() || !handler) {
		return 0;
	}
	const u64 id = _nextSubscription++;
	_subscriptions.push_back({id, e, std::move(type), std::move(handler)});
	return id;
}

void UISystem::Off(u64 id) {
	std::erase_if(_subscriptions, [&](const Subscription &s) { return s.id == id; });
}

bool UISystem::emit(Entity entity, const std::string &type, f32 value, std::string text,
	Vector2 delta) {
	UIEvent event{type, entity, entity, _pointer, value, std::move(text), false, delta};
	// Copy the route and IDs: handlers may destroy/reparent nodes or subscribe.
	std::vector<Entity> route;
	for (Entity e = entity; e.IsAlive(); e = e.GetParent()) {
		route.push_back(e);
	}
	for (Entity e : route) {
		if (!e.IsAlive()) {
			continue;
		}
		event.currentTarget = e;
		std::vector<u64> callbacks;
		for (const auto &s : _subscriptions) {
			if (s.entity == e && s.type == type) {
				callbacks.push_back(s.id);
			}
		}
		for (u64 id : callbacks) {
			const auto it = std::find_if(_subscriptions.begin(), _subscriptions.end(),
				[&](const Subscription &s) { return s.id == id; });
			if (it == _subscriptions.end()) {
				continue;
			}
			auto callback = it->handler;
			callback(event);
			if (event.handled || !entity.IsAlive()) {
				return true;
			}
		}
	}
	return false;
}

bool UISystem::Focus(Entity e) {
	if (e.IsAlive()) {
		const auto *n = find(e);
		const auto *c = e.Get<UIControl>();
		if (!n || !eligible(*n) || !c || !c->focusable || c->tabIndex < 0) {
			return false;
		}
	}
	if (_focus == e) {
		return true;
	}
	Entity old = _focus;
	_focus = e;
	if (old.IsAlive()) {
		if (auto *c = old.Get<UIControl>()) {
			c->focused = false;
		}
		emit(old, "blur");
	}
	if (e.IsAlive()) {
		if (auto *c = e.Get<UIControl>()) {
			c->focused = true;
		}
		emit(e, "focus");
		EnsureVisible(e);
	}
	return true;
}

void UISystem::moveFocus(bool backwards) {
	std::vector<Entity> controls;
	for (int i : _order) {
		const auto &n = _nodes[i];
		const auto *c = n.entity.Get<UIControl>();
		if (eligible(n) && c && c->focusable && c->tabIndex >= 0) {
			controls.push_back(n.entity);
		}
	}
	std::stable_sort(controls.begin(), controls.end(), [](Entity a, Entity b) {
		return a.Get<UIControl>()->tabIndex < b.Get<UIControl>()->tabIndex;
	});
	if (controls.empty()) {
		Focus({});
		return;
	}
	const auto it = std::find(controls.begin(), controls.end(), _focus);
	int index = it == controls.end() ? (backwards ? 0 : -1) : int(it - controls.begin());
	index = (index + (backwards ? -1 : 1) + controls.size()) % controls.size();
	Focus(controls[index]);
}

void UISystem::directionalFocus(Key key) {
	const auto *current = find(_focus);
	if (!current) {
		moveFocus(false);
		return;
	}
	const Vector2 origin = UITransformPoint(current->canvas.Get<UICanvas>()->layoutToViewport,
		{current->rect.x + current->rect.w / 2, current->rect.y + current->rect.h / 2});
	f32 best = std::numeric_limits<f32>::max();
	Entity target;
	for (int i : _order) {
		const auto &n = _nodes[i];
		const auto *c = n.entity.Get<UIControl>();
		if (!eligible(n) || !c || !c->focusable || c->tabIndex < 0 || n.entity == _focus) {
			continue;
		}
		const Vector2 center = UITransformPoint(n.canvas.Get<UICanvas>()->layoutToViewport,
			{n.rect.x + n.rect.w / 2, n.rect.y + n.rect.h / 2});
		const f32 dx = center.x - origin.x, dy = center.y - origin.y;
		const f32 main = key == Key::Left	 ? -dx
						 : key == Key::Right ? dx
						 : key == Key::Up	 ? -dy
											 : dy;
		if (main <= 0) {
			continue;
		}
		const f32 cross = key == Key::Left || key == Key::Right ? std::abs(dy) : std::abs(dx);
		const f32 score = main + cross * 3;
		if (score < best) {
			best = score;
			target = n.entity;
		}
	}
	if (target.IsAlive()) {
		Focus(target);
	}
}

void UISystem::Open(Entity e, bool open) {
	if (!e.IsAlive()) {
		return;
	}
	if (auto *p = e.Get<UIPopup>()) {
		if (p->open == open) {
			return;
		}
		p->open = open;
		emit(e, open ? "opened" : "closed");
	}
}

void UISystem::SetItems(Entity e, const std::vector<std::string> &items) {
	if (!e.IsAlive()) {
		return;
	}
	std::string text;
	for (size_t i = 0; i < items.size(); ++i) {
		if (i) {
			text += '\n';
		}
		text += items[i];
	}
	if (auto *l = e.Get<UIList>()) {
		l->items = text;
		l->selected = std::clamp(l->selected, -1, int(items.size()) - 1);
	}
	if (auto *d = e.Get<UIDropdown>()) {
		d->items = text;
		d->selected = std::clamp(d->selected, -1, int(items.size()) - 1);
	}
	if (auto *t = e.Get<UITabs>()) {
		t->titles = text;
		t->selected = std::clamp(t->selected, 0, std::max(0, int(items.size()) - 1));
	}
}

void UISystem::ScrollTo(Entity e, Vector2 offset) {
	if (auto *n = find(e)) {
		if (auto *s = e.Get<UIScrollView>()) {
			s->offset = {s->horizontal ? std::clamp(offset.x, 0.f,
											 std::max(0.f, s->contentSize.x - n->rect.w / n->scale))
									   : 0,
				s->vertical ? std::clamp(offset.y, 0.f,
								  std::max(0.f, s->contentSize.y - n->rect.h / n->scale))
							: 0};
		}
	}
}

void UISystem::EnsureVisible(Entity e) {
	const auto *n = find(e);
	if (!n) {
		return;
	}
	Rect target = n->rect;
	for (Entity p = e.GetParent(); p.IsAlive(); p = p.GetParent()) {
		const auto *pn = find(p);
		if (pn && pn->canvas != n->canvas) {
			break;
		}
		auto *scroll = p.Get<UIScrollView>();
		if (!pn || !scroll) {
			continue;
		}
		Vector2 delta;
		if (target.x < pn->rect.x) {
			delta.x = target.x - pn->rect.x;
		} else if (target.Right() > pn->rect.Right()) {
			delta.x = target.Right() - pn->rect.Right();
		}
		if (target.y < pn->rect.y) {
			delta.y = target.y - pn->rect.y;
		} else if (target.Top() > pn->rect.Top()) {
			delta.y = target.Top() - pn->rect.Top();
		}
		const Vector2 before = scroll->offset;
		ScrollTo(p, before + delta / pn->scale);
		const Vector2 movement = (scroll->offset - before) * pn->scale;
		target.x -= movement.x;
		target.y -= movement.y;
	}
}

void UISystem::changeSelection(Entity e, i32 selected) {
	if (!e.IsAlive()) {
		return;
	}
	i32 *value = nullptr;
	std::string labels;
	if (auto *l = e.Get<UIList>()) {
		value = &l->selected;
		labels = l->items;
	}
	if (auto *d = e.Get<UIDropdown>()) {
		value = &d->selected;
		labels = d->items;
	}
	if (auto *t = e.Get<UITabs>()) {
		value = &t->selected;
		labels = t->titles;
	}
	if (!value) {
		return;
	}
	const auto items = UIItems(labels);
	selected = items.empty() ? -1 : std::clamp(selected, 0, int(items.size()) - 1);
	if (*value == selected) {
		return;
	}
	*value = selected;
	emit(e, "changed", selected, selected >= 0 ? items[selected] : std::string{});
}

void UISystem::activate(Entity e) {
	if (!e.IsAlive()) {
		return;
	}
	if (auto *toggle = e.Get<UIToggle>()) {
		toggle->checked = !toggle->checked;
		const bool checked = toggle->checked;
		emit(e, "changed", checked ? 1 : 0);
	} else if (auto *d = e.Get<UIDropdown>()) {
		d->open = !d->open;
		if (d->open) {
			if (_dropdown.IsAlive() && _dropdown != e) {
				if (auto *old = _dropdown.Get<UIDropdown>()) {
					old->open = false;
				}
			}
			_dropdown = e;
		} else {
			_dropdown = {};
		}
	}
	if (e.IsAlive()) {
		emit(e, "clicked");
	}
}

void UISystem::dragValue(Entity e, Vector2 p) {
	const auto *n = find(e);
	if (!n) {
		return;
	}
	p = toLayout(*n, p);
	if (auto *slider = e.Get<UISlider>()) {
		if (slider->readOnly) {
			return;
		}
		f32 t = slider->vertical ? 1 - (p.y - n->rect.y) / std::max(1.f, n->rect.h)
								 : (p.x - n->rect.x) / std::max(1.f, n->rect.w);
		if (!std::isfinite(slider->minimum) || !std::isfinite(slider->maximum) ||
			slider->maximum < slider->minimum) {
			return;
		}
		f32 value = slider->minimum + std::clamp(t, 0.f, 1.f) * (slider->maximum - slider->minimum);
		if (std::isfinite(slider->step) && slider->step > 0) {
			value = slider->minimum +
					std::round((value - slider->minimum) / slider->step) * slider->step;
		}
		value = std::clamp(value, slider->minimum, slider->maximum);
		if (slider->value != value) {
			slider->value = value;
			emit(e, "changed", value);
		}
	} else if (e.Has<UIScrollView>() && _dragPart) {
		auto *scroll = e.Get<UIScrollView>();
		const bool vertical = _dragPart == 2;
		const f32 viewport = vertical ? n->rect.h : n->rect.w;
		const f32 content = (vertical ? scroll->contentSize.y : scroll->contentSize.x) * n->scale;
		const f32 thumb =
			std::max(20.f * n->scale, viewport * viewport / std::max(viewport, content));
		const f32 delta =
			(vertical ? p.y - _dragStart.y : p.x - _dragStart.x) / std::max(1.f, viewport - thumb);
		Vector2 offset = _dragOffset;
		if (vertical) {
			offset.y += delta * std::max(0.f, content - viewport) / n->scale;
		} else {
			offset.x += delta * std::max(0.f, content - viewport) / n->scale;
		}
		ScrollTo(e, offset);
	} else if (auto *bar = e.Get<UIScrollbar>(); bar && bar->target.IsAlive()) {
		if (auto *target = find(bar->target)) {
			if (auto *scroll = bar->target.Get<UIScrollView>()) {
				const f32 t = bar->vertical ? (p.y - n->rect.y) / std::max(1.f, n->rect.h)
											: (p.x - n->rect.x) / std::max(1.f, n->rect.w);
				Vector2 offset = scroll->offset;
				if (bar->vertical) {
					offset.y =
						std::clamp(t, 0.f, 1.f) *
						std::max(0.f, scroll->contentSize.y - target->rect.h / target->scale);
				} else {
					offset.x =
						std::clamp(t, 0.f, 1.f) *
						std::max(0.f, scroll->contentSize.x - target->rect.w / target->scale);
				}
				ScrollTo(bar->target, offset);
			}
		}
	} else if (e.Has<UITextInput>()) {
		positionTextCaret(e, p);
	}
}

void UISystem::keyInput(const InputEvent &event) {
	if (event.type == InputEventType::Char) {
		editText(event);
		return;
	}
	if (event.type != InputEventType::Key || event.key.action == KeyAction::Release) {
		return;
	}
	const Key key = event.key.key;
	if (key == Key::Tab) {
		moveFocus((event.key.mods & 1) != 0);
		return;
	}
	if (key == Key::Escape) {
		if (_dropdown.IsAlive()) {
			if (auto *d = _dropdown.Get<UIDropdown>()) {
				d->open = false;
			}
			_dropdown = {};
			return;
		}
		for (auto it = _order.rbegin(); it != _order.rend(); ++it) {
			if (auto *p = _nodes[*it].entity.Get<UIPopup>(); p && p->closeOnEscape) {
				Open(_nodes[*it].entity, false);
				return;
			}
		}
		Focus({});
		return;
	}
	if (!_focus.IsAlive()) {
		return;
	}
	if (_focus.Has<UITextInput>()) {
		editText(event);
		return;
	}
	if (key == Key::Enter || key == Key::KeypadEnter || key == Key::Space) {
		activate(_focus);
		return;
	}
	const bool decrease = key == Key::Left || key == Key::Up,
			   increase = key == Key::Right || key == Key::Down;
	if (!decrease && !increase && key != Key::Home && key != Key::End) {
		return;
	}
	auto nextSelection = [&](i32 current, size_t count) {
		if (key == Key::Home) {
			return 0;
		}
		if (key == Key::End) {
			return static_cast<int>(count) - 1;
		}
		return current + (decrease ? -1 : 1);
	};
	if (auto *s = _focus.Get<UISlider>()) {
		if (s->readOnly || s->maximum < s->minimum) {
			return;
		}
		const f32 step = s->step > 0 ? s->step : (s->maximum - s->minimum) / 100;
		f32 value = std::clamp(s->value + (decrease ? -step : step), s->minimum, s->maximum);
		if (key == Key::Home) {
			value = s->minimum;
		}
		if (key == Key::End) {
			value = s->maximum;
		}
		if (value != s->value) {
			s->value = value;
			emit(_focus, "changed", value);
		}
	} else if (auto *l = _focus.Get<UIList>()) {
		changeSelection(_focus, nextSelection(l->selected, UIItems(l->items).size()));
		if (_focus.IsAlive()) {
			if (auto *n = find(_focus); n && _focus.Has<UIList>()) {
				l = _focus.Get<UIList>();
				l->scroll =
					std::max(0.f, l->selected * l->rowHeight - n->rect.h / n->scale + l->rowHeight);
			}
		}
	} else if (auto *d = _focus.Get<UIDropdown>()) {
		changeSelection(_focus, nextSelection(d->selected, UIItems(d->items).size()));
	} else if (auto *t = _focus.Get<UITabs>()) {
		changeSelection(_focus, t->selected + (decrease ? -1 : 1));
	} else {
		directionalFocus(key);
	}
}

} // namespace sowa
