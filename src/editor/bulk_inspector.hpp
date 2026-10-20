#pragma once

#include "editor/scene_commands.hpp"

namespace sowa::editor {
struct SharedInspectorField {
	std::string component;
	FieldInfo field;
};

EntitySelection InspectorTargets(const EntitySelection &selection);
bool CompatibleInspectorField(const FieldInfo &a, const FieldInfo &b);
std::vector<SharedInspectorField> SharedInspectorFields(const EntitySelection &selection);
std::vector<BulkFieldEdit> ReadBulkField(const EntitySelection &selection,
	const SharedInspectorField &field);
// Channel bits preserve every other vector/color coordinate for each recipient.
std::vector<BulkFieldEdit> AssignBulkValue(const std::vector<BulkFieldEdit> &originals,
	const FieldValue &value, u32 channels = ~0u);
bool PreviewBulkFieldEdit(const std::vector<BulkFieldEdit> &edits, bool restore = false);
} // namespace sowa::editor
