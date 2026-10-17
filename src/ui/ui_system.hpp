#pragma once

#include <functional>
#include <unordered_map>
#include <vector>

#include "core/input.hpp"
#include "ecs/query.hpp"
#include "ui/theme.hpp"

namespace sowa {
class Renderer2D;
class EntityPickMap;
class ResourceRegistry;
class Font;
struct UIPainter;

struct UIEvent {
	std::string type;
	Entity target, currentTarget;
	Vector2 position;
	f32 value = 0;
	std::string text;
	bool handled = false;
	Vector2 delta;
};

// Layout is also available headlessly. Input is explicit so editor authoring
// can preview without activating controls. Call Layout again after gameplay.
class UISystem {
  public:
	using EventHandler = std::function<void(UIEvent &)>;
	void Init(World &world, ResourceRegistry &resources);
	void Layout(Vector2 viewportSize);
	void Layout(Vector2 viewportSize, const glm::mat4 &worldViewProjection);
	void HandleInput(Input &input, f32 dt, bool active = true);
	void Draw(Renderer2D &renderer, EntityPickMap &picks);
	void Reset();

	void QueueInput(const InputEvent &event) {
		_inputEvents.push_back(event);
	}

	void SetDefaultFont(ResourceID font) {
		_defaultFont = font;
	}

	void SetClipboard(std::function<std::string()> read,
		std::function<void(const std::string &)> write) {
		_readClipboard = std::move(read);
		_writeClipboard = std::move(write);
	}

	Entity HitTest(Vector2 viewportPoint) const; // top-left/Y-down

	Entity Focused() const {
		return _focus.IsAlive() ? _focus : Entity{};
	}

	bool Focus(Entity entity);

	bool CapturesPointer() const {
		return _pointerConsumed;
	}

	bool CapturesKeyboard() const {
		return _keyboardConsumed;
	}

	u64 On(Entity entity, std::string type, EventHandler handler);
	void Off(u64 subscription);
	void ScrollTo(Entity entity, Vector2 offset);
	void EnsureVisible(Entity entity);
	void SetItems(Entity entity, const std::vector<std::string> &items);
	void Open(Entity entity, bool open = true);

  private:
	struct Node {
		Entity entity;
		Entity canvas;
		int parent = -1;
		std::vector<int> children;
		Rect rect, clip;
		Vector2 desired;
		f32 scale = 1;
		i32 canvasOrder = 0, overlay = 0;
		bool enabled = true;
		ResourceID theme;
		std::string variant;
		Color tint;
		UIAppearance style;
	};

	struct Subscription {
		u64 id;
		Entity entity;
		std::string type;
		EventHandler handler;
	};

	int gather(Entity entity, int parent, const Node &inherited);
	Vector2 measure(int index, Vector2 available);
	void arrange(int index, Rect rect, Rect clip);
	Rect anchored(const Node &node, Rect parent) const;
	Vector2 toLayout(const Node &node, Vector2 point) const;
	void buildPaintOrder();
	Node *find(Entity entity);
	const Node *find(Entity entity) const;
	bool eligible(const Node &node) const;
	bool emit(Entity entity, const std::string &type, f32 value = 0, std::string text = {},
		Vector2 delta = {});
	void activate(Entity entity);
	void keyInput(const InputEvent &event);
	void editText(const InputEvent &event);
	void positionTextCaret(Entity entity, Vector2 point);
	void pointerButton(const InputEvent &event, Entity &hit);
	void scrollInput(const InputEvent &event, Entity hit);
	void dragValue(Entity entity, Vector2 point);
	void changeSelection(Entity entity, i32 selected);
	void moveFocus(bool backwards);
	void directionalFocus(Key key);
	Rect dropdownRect(const Node &node) const;
	int scrollbarPart(const Node &node, Vector2 point) const;
	void drawNode(const Node &node, Renderer2D &renderer, u32 token);
	void drawTextInput(const Node &node, UIPainter &paint, UITextInput &input);
	UIPainter painter(const Node &node, Renderer2D &renderer, u32 token) const;
	const Font *fontFor(const Node &node, ResourceID override = {}) const;

	ResourceRegistry *_resources = nullptr;
	Query<UICanvas> _canvases;
	std::vector<Node> _nodes;
	std::vector<int> _order;

	// Identical paint and hit-test order, including scrollbars above their contents.
	struct PaintEntry {
		int node;
		bool scrollbars;
	};

	std::vector<PaintEntry> _paintOrder;
	std::unordered_map<Entity, int> _indices;
	Vector2 _viewport{1, 1}, _pointer;
	ResourceID _defaultFont;
	Entity _focus, _hover, _capture, _modal, _lastModal, _dropdown;
	std::vector<std::pair<Entity, Entity>> _modalFocus;
	f32 _hoverTime = 0;
	bool _pointerConsumed = false, _keyboardConsumed = false;
	// Scrollbar dragging: 0 = control, 1 = horizontal bar, 2 = vertical bar.
	int _dragPart = 0;
	Vector2 _dragStart, _dragOffset;
	std::vector<InputEvent> _inputEvents;
	std::vector<Subscription> _subscriptions;
	u64 _nextSubscription = 1;
	std::function<std::string()> _readClipboard;
	std::function<void(const std::string &)> _writeClipboard;
};

std::vector<std::string> UIItems(const std::string &text);
Rect UIIntersect(Rect a, Rect b);
f32 UIRangeRatio(f32 value, f32 minimum, f32 maximum);
} // namespace sowa
