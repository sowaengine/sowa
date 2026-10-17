#pragma once

#include <string>
#include <vector>

#include "imgui.h"

namespace sowa {
class Entity;
struct FieldInfo;

namespace editor {

struct TypeAppearance {
	const char *icon;
	ImVec4 color;
	const char *label;
};

TypeAppearance EntityAppearance(Entity entity);
void InspectorFieldRow(const char *id, const std::string &label);
bool BeginInspectorTable(const char *id, float labelWidth);
bool IconActionButton(const char *icon, const char *tooltip, ImVec2 size = ImVec2(34.f, 26.f));
float InspectorLabelWidth(const std::string &nameLabel,
	const std::vector<const FieldInfo *> &fields);

} // namespace editor
} // namespace sowa
