#include "editor/bulk_inspector.hpp"

#include <algorithm>

#include "editor/inspector.hpp"

namespace sowa::editor {
namespace {

ComponentType component(Entity entity, std::string_view name) {
	for (auto type : entity.GetComponents()) {
		if (type.Name() == name) {
			return type;
		}
	}
	return {};
}

FieldValue channels(const FieldValue &original, const FieldValue &value, u32 mask) {
	if (original.index() != value.index()) {
		return original;
	}
	return std::visit(
		[&](const auto &before) -> FieldValue {
			using T = std::decay_t<decltype(before)>;
			const T &after = std::get<T>(value);
			T result = before;
			if constexpr (std::is_same_v<T, Vector2> || std::is_same_v<T, Vector3>) {
				if (mask & 1) {
					result.x = after.x;
				}
				if (mask & 2) {
					result.y = after.y;
				}
				if constexpr (std::is_same_v<T, Vector3>) {
					if (mask & 4) {
						result.z = after.z;
					}
				}
			} else if constexpr (std::is_same_v<T, Color>) {
				if (mask & 1) {
					result.r = after.r;
				}
				if (mask & 2) {
					result.g = after.g;
				}
				if (mask & 4) {
					result.b = after.b;
				}
				if (mask & 8) {
					result.a = after.a;
				}
			} else if (mask) {
				result = after;
			}
			return result;
		},
		original);
}
} // namespace

EntitySelection InspectorTargets(const EntitySelection &selection) {
	std::vector<Entity> editable;
	for (Entity entity : selection.Members()) {
		if (CanEdit(entity, false)) {
			editable.push_back(entity);
		}
	}
	EntitySelection targets;
	targets.Assign(SelectionRoots(editable), selection.ActiveRoot());
	return targets;
}

bool CompatibleInspectorField(const FieldInfo &a, const FieldInfo &b) {
	if (a.name != b.name || a.type != b.type || !b.IsExposed() || a.enumName != b.enumName ||
		a.resourceType != b.resourceType || a.enumConstants.size() != b.enumConstants.size()) {
		return false;
	}
	for (std::size_t i = 0; i < a.enumConstants.size(); ++i) {
		if (a.enumConstants[i].name != b.enumConstants[i].name ||
			a.enumConstants[i].value != b.enumConstants[i].value) {
			return false;
		}
	}
	return true;
}

std::vector<SharedInspectorField> SharedInspectorFields(const EntitySelection &selection) {
	std::vector<SharedInspectorField> fields;
	if (selection.Members().empty()) {
		return fields;
	}
	for (auto type : selection.Members().front().GetComponents()) {
		for (const auto *field : ExposedFields(type)) {
			const bool shared = std::all_of(selection.Members().begin(), selection.Members().end(),
				[&](Entity entity) {
					const auto other = component(entity, type.Name());
					const auto *candidate = other.FindField(field->name);
					return entity.Has(type) && candidate &&
						CompatibleInspectorField(*field, *candidate);
				});
			if (shared) {
				fields.push_back({type.Name(), *field});
			}
		}
	}
	return fields;
}

std::vector<BulkFieldEdit> ReadBulkField(const EntitySelection &selection,
	const SharedInspectorField &field) {
	std::vector<BulkFieldEdit> values;
	for (Entity entity : selection.Members()) {
		const auto type = component(entity, field.component);
		const auto *current = type.FindField(field.field.name);
		FieldValue value;
		if (!current || !CompatibleInspectorField(field.field, *current) ||
			!CaptureFieldValue(entity, type, *current, value)) {
			return {};
		}
		values.push_back({entity, field.component, field.field.name, value, value, *current});
	}
	return values;
}

std::vector<BulkFieldEdit> AssignBulkValue(const std::vector<BulkFieldEdit> &originals,
	const FieldValue &value, u32 mask) {
	auto edits = originals;
	for (auto &edit : edits) {
		edit.after = channels(edit.before, value, mask);
	}
	return edits;
}

bool PreviewBulkFieldEdit(const std::vector<BulkFieldEdit> &edits, bool restore) {
	for (const auto &edit : edits) {
		const auto type = component(edit.entity, edit.component);
		const auto *field = type.FindField(edit.field);
		FieldValue current;
		if (!edit.entity.IsAlive() || !field ||
			(edit.schema && !CompatibleInspectorField(*edit.schema, *field)) ||
			!CaptureFieldValue(edit.entity, type, *field, current) ||
			current.index() != edit.before.index() || current.index() != edit.after.index()) {
			return false;
		}
	}
	for (const auto &edit : edits) {
		const auto type = component(edit.entity, edit.component);
		auto ref = Entity(edit.entity).GetField(type, edit.field);
		const auto &value = restore ? edit.before : edit.after;
		if (ref.Type() == FieldType::Enum) {
			*static_cast<i32 *>(ref.Data()) = std::get<i32>(value);
		} else {
			std::visit([&](const auto &v) { ref.Set(v); }, value);
		}
	}
	return true;
}
} // namespace sowa::editor
