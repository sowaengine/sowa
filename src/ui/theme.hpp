#pragma once

#include "resource/resource.hpp"
#include "ui/components.hpp"
#include <vector>

namespace sowa {
class ResourceRegistry;

struct UIAppearance {
	Color background{0.16f, 0.19f, 0.25f, 1};
	Color hovered{0.23f, 0.28f, 0.36f, 1};
	Color pressed{0.10f, 0.13f, 0.18f, 1};
	Color disabled{0.16f, 0.17f, 0.19f, 0.6f};
	Color text{0.92f, 0.95f, 1, 1};
	Color accent{0.28f, 0.64f, 1, 1};
	Color border{0.33f, 0.39f, 0.48f, 1};
	ResourceID font, texture;
	f32 fontSize = 22, borderWidth = 1;
	Vector2 padding{12, 8};
	UIImageMode imageMode = UIImageMode::NineSlice;
	Vector2 regionPosition{0, 0}, regionSize{0, 0}, borderMin{8, 8}, borderMax{8, 8};
	bool tileCenter = false, tileEdges = false;
};

struct UIThemeRule {
	std::string role, variant;
	UIAppearance appearance;
};

class UITheme : public Resource, public UIAppearance {
  public:
	std::vector<UIThemeRule> rules;
	UIAppearance Resolve(const std::string &role, const std::string &variant) const;
	void Serialize(Archive &archive) override;
};

void RegisterUITheme(ResourceRegistry &resources);
std::string UIRole(Entity entity);
} // namespace sowa
