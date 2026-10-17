#include "ui/components.hpp"
#include "ecs/components/transform2d.hpp"
#include "ecs/world.hpp"
#include <algorithm>

namespace sowa {
void RegisterUIComponents(World &world) {
	world.RegisterComponent<UICanvas>("UICanvas")
		.Field("space", &UICanvas::space)
		.Field("scaleMode", &UICanvas::scaleMode)
		.Field("referenceSize", &UICanvas::referenceSize)
		.Field("safePadding", &UICanvas::safePadding)
		.ResourceField("theme", &UICanvas::theme, "UITheme")
		.Field("sortOrder", &UICanvas::sortOrder);
	world.RegisterComponent<UIRect>("UIRect")
		.Field("anchorMin", &UIRect::anchorMin)
		.Field("anchorMax", &UIRect::anchorMax)
		.Field("pivot", &UIRect::pivot)
		.Field("position", &UIRect::position)
		.Field("size", &UIRect::size)
		.Field("minSize", &UIRect::minSize)
		.Field("maxSize", &UIRect::maxSize)
		.Field("margin", &UIRect::margin)
		.Field("fitContentX", &UIRect::fitContentX)
		.Field("fitContentY", &UIRect::fitContentY)
		.Field("grow", &UIRect::grow)
		.Field("clipContents", &UIRect::clipContents);
	world.RegisterComponent<UILayout>("UILayout")
		.Field("mode", &UILayout::mode)
		.Field("padding", &UILayout::padding)
		.Field("spacing", &UILayout::spacing)
		.Field("alignment", &UILayout::alignment)
		.Field("columns", &UILayout::columns);
	world.RegisterComponent<UIControl>("UIControl")
		.Field("disabled", &UIControl::disabled)
		.Field("focusable", &UIControl::focusable)
		.Field("blockPointer", &UIControl::blockPointer)
		.Field("tabIndex", &UIControl::tabIndex)
		.Field("hovered", &UIControl::hovered, FieldFlags::Expose)
		.Field("pressed", &UIControl::pressed, FieldFlags::Expose)
		.Field("focused", &UIControl::focused, FieldFlags::Expose);
	world.RegisterComponent<UIStyle>("UIStyle")
		.ResourceField("theme", &UIStyle::theme, "UITheme")
		.Field("variant", &UIStyle::variant)
		.Field("modulate", &UIStyle::modulate);
	world.RegisterComponent<UIPanel>("UIPanel").Field("modulate", &UIPanel::modulate);
	world.RegisterComponent<UINinePatchRect>("UINinePatchRect")
		.ResourceField("texture", &UINinePatchRect::texture, "ImageTexture")
		.Field("paddingLeft", &UINinePatchRect::paddingLeft)
		.Field("paddingRight", &UINinePatchRect::paddingRight)
		.Field("paddingTop", &UINinePatchRect::paddingTop)
		.Field("paddingBottom", &UINinePatchRect::paddingBottom)
		.Field("tileCenter", &UINinePatchRect::tileCenter)
		.Field("tileEdges", &UINinePatchRect::tileEdges)
		.Field("drawCenter", &UINinePatchRect::drawCenter)
		.Field("modulate", &UINinePatchRect::modulate);
	world.RegisterComponent<UIImage>("UIImage")
		.ResourceField("texture", &UIImage::texture, "ImageTexture")
		.Field("regionPosition", &UIImage::regionPosition)
		.Field("regionSize", &UIImage::regionSize)
		.Field("borderMin", &UIImage::borderMin)
		.Field("borderMax", &UIImage::borderMax)
		.Field("mode", &UIImage::mode)
		.Field("tileCenter", &UIImage::tileCenter)
		.Field("tileEdges", &UIImage::tileEdges)
		.Field("modulate", &UIImage::modulate);
	world.RegisterComponent<UILabel>("UILabel")
		.Field("text", &UILabel::text)
		.ResourceField("font", &UILabel::font, "Font")
		.Field("fontSize", &UILabel::fontSize)
		.Field("lineSpacing", &UILabel::lineSpacing)
		.Field("wrap", &UILabel::wrap)
		.Field("alignment", &UILabel::alignment)
		.Field("verticalAlignment", &UILabel::verticalAlignment)
		.Field("overflow", &UILabel::overflow)
		.Field("modulate", &UILabel::modulate);
	world.RegisterComponent<UIButton>("UIButton").Field("text", &UIButton::text);
	world.RegisterComponent<UIToggle>("UIToggle")
		.Field("text", &UIToggle::text)
		.Field("checked", &UIToggle::checked);
	world.RegisterComponent<UISlider>("UISlider")
		.Field("value", &UISlider::value)
		.Field("minimum", &UISlider::minimum)
		.Field("maximum", &UISlider::maximum)
		.Field("step", &UISlider::step)
		.Field("vertical", &UISlider::vertical)
		.Field("readOnly", &UISlider::readOnly);
	world.RegisterComponent<UIProgressBar>("UIProgressBar")
		.Field("value", &UIProgressBar::value)
		.Field("minimum", &UIProgressBar::minimum)
		.Field("maximum", &UIProgressBar::maximum)
		.Field("vertical", &UIProgressBar::vertical)
		.Field("showValue", &UIProgressBar::showValue);
	world.RegisterComponent<UIScrollView>("UIScrollView")
		.Field("offset", &UIScrollView::offset)
		.Field("horizontal", &UIScrollView::horizontal)
		.Field("vertical", &UIScrollView::vertical)
		.Field("showScrollbars", &UIScrollView::showScrollbars)
		.Field("wheelSpeed", &UIScrollView::wheelSpeed);
	world.RegisterComponent<UIScrollbar>("UIScrollbar")
		.Field("target", &UIScrollbar::target)
		.Field("vertical", &UIScrollbar::vertical);
	world.RegisterComponent<UITextInput>("UITextInput")
		.Field("text", &UITextInput::text)
		.Field("placeholder", &UITextInput::placeholder)
		.Field("multiline", &UITextInput::multiline)
		.Field("password", &UITextInput::password)
		.Field("readOnly", &UITextInput::readOnly)
		.Field("maxLength", &UITextInput::maxLength);
	world.RegisterComponent<UIList>("UIList")
		.Field("items", &UIList::items)
		.Field("selected", &UIList::selected)
		.Field("rowHeight", &UIList::rowHeight)
		.Field("scroll", &UIList::scroll);
	world.RegisterComponent<UIDropdown>("UIDropdown")
		.Field("items", &UIDropdown::items)
		.Field("selected", &UIDropdown::selected)
		.Field("maxVisibleRows", &UIDropdown::maxVisibleRows);
	world.RegisterComponent<UIPopup>("UIPopup")
		.Field("open", &UIPopup::open)
		.Field("modal", &UIPopup::modal)
		.Field("dismissOutside", &UIPopup::dismissOutside)
		.Field("closeOnEscape", &UIPopup::closeOnEscape);
	world.RegisterComponent<UITabs>("UITabs")
		.Field("titles", &UITabs::titles)
		.Field("selected", &UITabs::selected)
		.Field("headerHeight", &UITabs::headerHeight);
	world.RegisterComponent<UITooltip>("UITooltip")
		.Field("text", &UITooltip::text)
		.Field("delay", &UITooltip::delay);
}

void SetUIAnchorPreset(UIRect &rect, const std::string &preset) {
	if (preset == "Stretch") {
		rect.anchorMin = {0, 0};
		rect.anchorMax = {1, 1};
		rect.pivot = {0, 0};
		rect.position = {0, 0};
		rect.size = {0, 0};
	} else {
		Vector2 anchor{0, 0};
		if (preset == "Center") {
			anchor = {0.5f, 0.5f};
		} else if (preset == "TopRight") {
			anchor = {1, 0};
		} else if (preset == "BottomLeft") {
			anchor = {0, 1};
		} else if (preset == "BottomRight") {
			anchor = {1, 1};
		}
		rect.anchorMin = rect.anchorMax = rect.pivot = anchor;
		rect.position = {0, 0};
	}
}

Entity CreateUI(Entity parent, const std::string &kind, const std::string &name) {
	if (!parent.IsAlive()) {
		return {};
	}
	static const std::vector<std::string> kinds = {"UINinePatchRect", "UICanvas", "UIRect",
		"UIPanel", "UIImage", "UILabel", "UIButton", "UIToggle", "UISlider", "UIProgressBar",
		"UIScrollView", "UIScrollbar", "UITextInput", "UIList", "UIDropdown", "UIPopup", "UIModal",
		"UITabs", "UITooltip", "UIRow", "UIColumn", "UIGrid", "UIFlow", "UIOverlay"};
	if (std::find(kinds.begin(), kinds.end(), kind) == kinds.end()) {
		return {};
	}
	Entity child = parent.CreateChild(name.empty() ? kind : name);
	child.Add<UIRect>();
	if (kind == "UICanvas") {
		child.Add<UICanvas>();
		child.Add<Transform2D>();
		child.Get<UIRect>()->size = child.Get<UICanvas>()->referenceSize;
		child.Get<UIRect>()->pivot = {.5f, .5f};
		return child;
	}
	if (kind == "UIRect") {
		return child;
	}
	if (kind == "UIRow" || kind == "UIColumn" || kind == "UIGrid" || kind == "UIFlow" ||
		kind == "UIOverlay") {
		UILayout layout;
		layout.mode = UILayoutMode::Vertical;
		if (kind == "UIRow") {
			layout.mode = UILayoutMode::Horizontal;
		}
		if (kind == "UIGrid") {
			layout.mode = UILayoutMode::Grid;
		}
		if (kind == "UIFlow") {
			layout.mode = UILayoutMode::Flow;
		}
		if (kind == "UIOverlay") {
			layout.mode = UILayoutMode::Overlay;
		}
		child.Add(layout);
		child.Get<UIRect>()->size = {400, 300};
		return child;
	}
	if (kind == "UIPanel") {
		child.Add<UIPanel>();
	} else if (kind == "UINinePatchRect") {
		child.Add<UINinePatchRect>();
	} else if (kind == "UIImage") {
		child.Add<UIImage>();
	} else if (kind == "UILabel") {
		child.Add<UILabel>();
	} else if (kind == "UIButton") {
		child.Add<UIButton>();
	} else if (kind == "UIToggle") {
		child.Add<UIToggle>();
	} else if (kind == "UISlider") {
		child.Add<UISlider>();
	} else if (kind == "UIProgressBar") {
		child.Add<UIProgressBar>();
	} else if (kind == "UIScrollView") {
		child.Add<UIScrollView>();
	} else if (kind == "UIScrollbar") {
		child.Add<UIScrollbar>();
	} else if (kind == "UITextInput") {
		child.Add<UITextInput>();
	} else if (kind == "UIList") {
		child.Add<UIList>();
	} else if (kind == "UIDropdown") {
		child.Add<UIDropdown>();
	} else if (kind == "UIPopup") {
		child.Add<UIPopup>();
	} else if (kind == "UITabs") {
		child.Add<UITabs>();
	} else if (kind == "UITooltip") {
		child.Add<UITooltip>();
	} else if (kind == "UIModal") {
		child.Add(UIPopup{false, true});
		child.Add<UIPanel>();
	}
	if (kind == "UIButton" || kind == "UIToggle" || kind == "UISlider" || kind == "UIScrollView" ||
		kind == "UIScrollbar" || kind == "UITextInput" || kind == "UIList" ||
		kind == "UIDropdown" || kind == "UITabs") {
		child.Add<UIControl>();
	}
	if (kind == "UITooltip") {
		child.Add<UIControl>();
		child.Get<UIControl>()->focusable = false;
	}
	if (kind == "UIScrollView") {
		child.Add<UILayout>();
		child.Get<UIRect>()->clipContents = true;
	}
	if (kind == "UIList" || kind == "UIScrollView" || kind == "UITabs") {
		child.Get<UIRect>()->size = {300, 220};
	}
	if (kind == "UIPopup" || kind == "UIModal") {
		child.Get<UIRect>()->size = {420, 280};
		SetUIAnchorPreset(*child.Get<UIRect>(), "Center");
		child.Add<UIPanel>();
	}
	if (kind == "UIScrollbar") {
		child.Get<UIRect>()->size = {16, 160};
	}
	return child;
}
} // namespace sowa
