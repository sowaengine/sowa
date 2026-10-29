#include "ui/theme.hpp"
#include "core/serialize/archive.hpp"
#include "resource/resource_registry.hpp"

namespace sowa {
namespace {
template <class M> M UITheme::*themeMember(M UIAppearance::*member) {
	return member;
}

void appearance(Archive &a, UIAppearance &v) {
	a.Field("background", v.background);
	a.Field("hovered", v.hovered);
	a.Field("pressed", v.pressed);
	a.Field("disabled", v.disabled);
	a.Field("text", v.text);
	a.Field("accent", v.accent);
	a.Field("border", v.border);
	a.ResourceField("font", v.font.value);
	a.ResourceField("texture", v.texture.value);
	a.Field("fontSize", v.fontSize);
	a.Field("borderWidth", v.borderWidth);
	a.Field("padding", v.padding);
	a.EnumField("imageMode", v.imageMode, EnumTraits<UIImageMode>::Constants());
	a.Field("regionPosition", v.regionPosition);
	a.Field("regionSize", v.regionSize);
	a.Field("borderMin", v.borderMin);
	a.Field("borderMax", v.borderMax);
	a.Field("tileCenter", v.tileCenter);
	a.Field("tileEdges", v.tileEdges);
}
} // namespace

UIAppearance UITheme::Resolve(const std::string &role, const std::string &variant) const {
	UIAppearance result = *this;
	for (const auto &rule : rules) {
		if (rule.role == role && rule.variant.empty()) {
			result = rule.appearance;
		}
	}
	if (!variant.empty()) {
		for (const auto &rule : rules) {
			if (rule.role == role && rule.variant == variant) {
				result = rule.appearance;
			}
		}
	}
	return result;
}

void UITheme::Serialize(Archive &archive) {
	appearance(archive, *this);
	archive.List("rules", rules, [&](Archive &a, UIThemeRule &rule) {
		if (a.IsLoading()) {
			rule.appearance = *this;
		}
		a.Field("role", rule.role);
		a.Field("variant", rule.variant);
		appearance(a, rule.appearance);
	});
}

void RegisterUITheme(ResourceRegistry &resources) {
	resources.RegisterType<UITheme>("UITheme")
		.Field("background", themeMember(&UIAppearance::background))
		.Field("hovered", themeMember(&UIAppearance::hovered))
		.Field("pressed", themeMember(&UIAppearance::pressed))
		.Field("disabled", themeMember(&UIAppearance::disabled))
		.Field("text", themeMember(&UIAppearance::text))
		.Field("accent", themeMember(&UIAppearance::accent))
		.Field("border", themeMember(&UIAppearance::border))
		.Field("fontSize", themeMember(&UIAppearance::fontSize))
		.Field("borderWidth", themeMember(&UIAppearance::borderWidth))
		.Field("padding", themeMember(&UIAppearance::padding))
		.ResourceField("font", themeMember(&UIAppearance::font), "Font")
		.ResourceField("texture", themeMember(&UIAppearance::texture), "ImageTexture")
		.Field("imageMode", themeMember(&UIAppearance::imageMode))
		.Field("regionPosition", themeMember(&UIAppearance::regionPosition))
		.Field("regionSize", themeMember(&UIAppearance::regionSize))
		.Field("borderMin", themeMember(&UIAppearance::borderMin))
		.Field("borderMax", themeMember(&UIAppearance::borderMax))
		.Field("tileCenter", themeMember(&UIAppearance::tileCenter))
		.Field("tileEdges", themeMember(&UIAppearance::tileEdges));
}

std::string UIRole(Entity e) {
	if (e.Has<UIButton>()) {
		return "Button";
	}
	if (e.Has<UIToggle>()) {
		return "Toggle";
	}
	if (e.Has<UISlider>()) {
		return "Slider";
	}
	if (e.Has<UIProgressBar>()) {
		return "ProgressBar";
	}
	if (e.Has<UITextInput>()) {
		return "TextInput";
	}
	if (e.Has<UIList>()) {
		return "List";
	}
	if (e.Has<UIDropdown>()) {
		return "Dropdown";
	}
	if (e.Has<UITabs>()) {
		return "Tabs";
	}
	if (e.Has<UILabel>()) {
		return "Label";
	}
	return "Panel";
}
} // namespace sowa
