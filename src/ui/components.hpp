#pragma once

#include <array>
#include <glm/glm.hpp>
#include <string>

#include "core/reflection/field_type.hpp"
#include "ecs/entity.hpp"
#include "math/color.hpp"
#include "math/rect.hpp"
#include "math/vector2.hpp"
#include "render/text_layout.hpp"
#include "resource/resource_id.hpp"

namespace sowa {
class World;

enum class UISpace {
	World,
	Screen
};
SOWA_REFLECT_ENUM(UISpace, {"World", UISpace::World}, {"Screen", UISpace::Screen});

enum class UIScaleMode {
	Viewport,
	FitReference
};
SOWA_REFLECT_ENUM(UIScaleMode, {"Viewport", UIScaleMode::Viewport},
	{"FitReference", UIScaleMode::FitReference});
enum class UILayoutMode {
	Free,
	Horizontal,
	Vertical,
	Grid,
	Flow,
	Overlay
};
SOWA_REFLECT_ENUM(UILayoutMode, {"Free", UILayoutMode::Free},
	{"Horizontal", UILayoutMode::Horizontal}, {"Vertical", UILayoutMode::Vertical},
	{"Grid", UILayoutMode::Grid}, {"Flow", UILayoutMode::Flow}, {"Overlay", UILayoutMode::Overlay});
enum class UIAlignment {
	Start,
	Center,
	End,
	Stretch
};
SOWA_REFLECT_ENUM(UIAlignment, {"Start", UIAlignment::Start}, {"Center", UIAlignment::Center},
	{"End", UIAlignment::End}, {"Stretch", UIAlignment::Stretch});
enum class UIImageMode {
	Stretch,
	Native,
	Tile,
	NineSlice
};
SOWA_REFLECT_ENUM(UIImageMode, {"Stretch", UIImageMode::Stretch}, {"Native", UIImageMode::Native},
	{"Tile", UIImageMode::Tile}, {"NineSlice", UIImageMode::NineSlice});
enum class UIOverflow {
	Clip,
	Ellipsis,
	Visible
};
SOWA_REFLECT_ENUM(UIOverflow, {"Clip", UIOverflow::Clip}, {"Ellipsis", UIOverflow::Ellipsis},
	{"Visible", UIOverflow::Visible});

struct UICanvas {
	UISpace space = UISpace::World;
	UIScaleMode scaleMode = UIScaleMode::FitReference;
	Vector2 referenceSize{1280, 720};
	Vector2 safePadding{0, 0};
	ResourceID theme;
	i32 sortOrder = 0;

	// Derived once per layout; drawing, input and editor handles share this mapping.
	glm::mat4 projection{1.f};
	glm::mat4 layoutToViewport{1.f};
	glm::mat4 viewportToLayout{1.f};
	Rect bounds;
	f32 unitScale = 1.f;
};

// Rectangles use canvas-local, top-left/Y-down coordinates. Canvas transforms
// place the entire hierarchy in the world or in screen space.
struct UIRect {
	Vector2 anchorMin{0, 0}, anchorMax{0, 0};
	Vector2 pivot{0, 0}, position{0, 0}, size{160, 40};
	Vector2 minSize{0, 0}, maxSize{0, 0};
	Vector2 margin{0, 0};
	bool fitContentX = false, fitContentY = false;
	f32 grow = 0;
	bool clipContents = false;
	Rect resolved;
	std::array<Vector2, 4> viewportCorners{}; // top-left, top-right, bottom-right, bottom-left
	Vector2 measured;
	Vector2 anchorSpan; // resolved anchor contribution in authored units, used by resizing
};

struct UILayout {
	UILayoutMode mode = UILayoutMode::Vertical;
	Vector2 padding{12, 12}, spacing{8, 8};
	UIAlignment alignment = UIAlignment::Stretch;
	i32 columns = 2;
};

struct UIControl {
	bool disabled = false, focusable = true, blockPointer = true;
	i32 tabIndex = 0;
	// Runtime state, not serialized.
	bool hovered = false, pressed = false, focused = false;
};

struct UIStyle {
	ResourceID theme;
	std::string variant;
	Color modulate;
};

struct UIPanel {
	Color modulate;
};

struct UIImage {
	ResourceID texture;
	Vector2 regionPosition{0, 0}, regionSize{0, 0};
	Vector2 borderMin{8, 8}, borderMax{8, 8};
	UIImageMode mode = UIImageMode::Stretch;
	bool tileCenter = false, tileEdges = false;
	Color modulate;
};

// Uses UIRect for placement and size. Padding defines fixed edges in texture pixels.
struct UINinePatchRect {
	ResourceID texture;
	f32 paddingLeft = 8;
	f32 paddingRight = 8;
	f32 paddingTop = 8;
	f32 paddingBottom = 8;
	bool tileCenter = false;
	bool tileEdges = false;
	bool drawCenter = true;
	Color modulate;
};

struct UILabel {
	std::string text = "Label";
	ResourceID font;
	f32 fontSize = 0, lineSpacing = 1;
	bool wrap = false;
	TextHorizontalAlignment alignment = TextHorizontalAlignment::Left;
	UIAlignment verticalAlignment = UIAlignment::Center;
	UIOverflow overflow = UIOverflow::Clip;
	Color modulate;
};

struct UIButton {
	std::string text = "Button";
};

struct UIToggle {
	std::string text = "Toggle";
	bool checked = false;
};

struct UISlider {
	f32 value = 0.5f, minimum = 0, maximum = 1, step = 0.01f;
	bool vertical = false, readOnly = false;
};

struct UIProgressBar {
	f32 value = 0.5f, minimum = 0, maximum = 1;
	bool vertical = false, showValue = true;
};

struct UIScrollView {
	Vector2 offset{0, 0};
	bool horizontal = false, vertical = true, showScrollbars = true;
	f32 wheelSpeed = 40;
	Vector2 contentSize; // resolved runtime extent
};

struct UIScrollbar {
	Entity target;
	bool vertical = true;
};

struct UITextInput {
	std::string text, placeholder = "Enter text...";
	bool multiline = false, password = false, readOnly = false;
	i32 maxLength = 0; // zero is unlimited, measured in codepoints
	// UTF-8 byte boundaries; maintained by the input system.
	u32 caret = 0, selectionAnchor = 0;
	Vector2 scroll;
};

struct UIList {
	// One label per line. Lua ui.set_items also accepts a string array.
	std::string items = "First\nSecond\nThird";
	i32 selected = -1;
	f32 rowHeight = 32, scroll = 0;
};

struct UIDropdown {
	std::string items = "First\nSecond\nThird";
	i32 selected = 0, maxVisibleRows = 6;
	bool open = false; // runtime
	f32 scroll = 0;
};

struct UIPopup {
	bool open = false, modal = false, dismissOutside = true, closeOnEscape = true;
};

struct UITabs {
	std::string titles = "First\nSecond";
	i32 selected = 0;
	f32 headerHeight = 40;
};

struct UITooltip {
	std::string text = "Tooltip";
	f32 delay = 0.5f;
};

void RegisterUIComponents(World &world);
// Convenient presets shared by editor and Lua, including required components.
Entity CreateUI(Entity parent, const std::string &kind, const std::string &name = {});
void SetUIAnchorPreset(UIRect &rect, const std::string &preset);
} // namespace sowa
