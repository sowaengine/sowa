#include "editor/editor.hpp"

#include <algorithm>

#include "editor/inspector.hpp"
#include "imgui.h"
#include "imgui_internal.h"
#include "misc/cpp/imgui_stdlib.h"
#include "resource/resource_registry.hpp"

namespace sowa {

void Editor::finishBulkInspector(bool cancel) {
	if (!_bulkPending) {
		return;
	}
	auto pending = std::move(*_bulkPending);
	_bulkPending.reset();
	if (cancel) {
		ImGui::ClearActiveID();
		editor::PreviewBulkFieldEdit(pending.edits, true);
		return;
	}
	if (!editor::RecordBulkFieldEdit(_sceneSaveStates, pending.scene, pending.edits)) {
		editor::PreviewBulkFieldEdit(pending.edits, true);
	}
}

void Editor::drawBulkInspector(ResourceRegistry &resources, bool playing) {
	auto targets = editor::InspectorTargets(selection());
	const bool editable = !playing && editor::CanEditSelection(targets, _editingScene);
	if (targets.Members().empty()) {
		targets = selection();
	}
	ImGui::Text("%zu entities selected", selection().Members().size());
	ImGui::TextDisabled("Active: %s", editor::DisplayName(targets.Active()).c_str());
	ImGui::Separator();
	std::string section;
	for (const auto &shared : editor::SharedInspectorFields(targets)) {
		auto values = editor::ReadBulkField(targets, shared);
		if (values.empty()) {
			continue;
		}
		if (section != shared.component) {
			section = shared.component;
			ImGui::SeparatorText(section.c_str());
		}
		ImGui::PushID(section.c_str());
		ImGui::PushID(shared.field.name.c_str());
		ImGui::TextUnformatted(editor::DisplayName(shared.field.name).c_str());
		const auto active = std::find_if(values.begin(), values.end(),
			[&](const auto &entry) { return entry.entity == targets.Active(); });
		editor::FieldValue value = active->before;
		bool changed = false, finished = false;
		ResourceID created;
		u32 mask = ~0u;
		const auto status = [&] { finished = finished || ImGui::IsItemDeactivatedAfterEdit(); };
		ImGui::BeginDisabled(!editable);
		switch (shared.field.type) {
		case FieldType::Bool: {
			changed = ImGui::Checkbox("##value", &std::get<bool>(value));
			status();
			break;
		}
		case FieldType::Enum: {
			const auto *name = shared.field.FindEnumName(std::get<i32>(value));
			if (ImGui::BeginCombo("##value", name ? name->c_str() : "Unknown")) {
				for (const auto &item : shared.field.enumConstants) {
					if (ImGui::Selectable(item.name.c_str(), std::get<i32>(value) == item.value)) {
						value = item.value;
						changed = finished = true;
					}
				}
				ImGui::EndCombo();
			}
			break;
		}
		case FieldType::String: {
			changed = ImGui::InputText("##value", &std::get<std::string>(value));
			status();
			break;
		}
		case FieldType::EntityRef: {
			Entity target = std::get<Entity>(value);
			std::string preview = target.IsAlive() ? target.GetPath() : "None";
			if (ImGui::BeginCombo("##value", preview.c_str())) {
				if (ImGui::Selectable("Clear")) {
					value = Entity{};
					changed = finished = true;
				}
				const auto visit = [&](const auto &self, Entity e) -> void {
					ImGui::PushID(reinterpret_cast<void *>(static_cast<uintptr_t>(e.Raw().id())));
					if (ImGui::Selectable(e.GetPath().c_str())) {
						value = e;
						changed = finished = true;
					}
					ImGui::PopID();
					for (Entity child : e.GetChildren()) {
						self(self, child);
					}
				};
				if (_editingScene.IsValid()) {
					visit(visit, _editingScene.Root());
				}
				ImGui::EndCombo();
			}
			break;
		}
		case FieldType::ResourceRef: {
			const auto label = editor::ResourceLabel(resources, std::get<ResourceID>(value));
			if (ImGui::BeginCombo("##value", label.c_str())) {
				for (const auto &type : resources.TypeNames()) {
					if (!shared.field.resourceType.empty() && type != shared.field.resourceType) {
						continue;
					}
					if (ImGui::Selectable(("New " + type).c_str())) {
						created = editor::CreateSceneResource(_editingScene, resources, type);
						if (created.IsValid()) {
							value = created;
							changed = finished = true;
						}
					}
				}
				if (ImGui::Selectable("Clear")) {
					value = ResourceID{};
					changed = finished = true;
				}
				for (auto id : editor::SceneResourcesOfType(_editingScene, resources,
						 shared.field.resourceType)) {
					ImGui::PushID(reinterpret_cast<void *>(static_cast<uintptr_t>(id.value)));
					if (ImGui::Selectable(editor::ResourceLabel(resources, id).c_str())) {
						value = id;
						changed = finished = true;
					}
					ImGui::PopID();
				}
				ImGui::EndCombo();
			}
			break;
		}
		case FieldType::Vector2:
		case FieldType::Vector3:
		case FieldType::Color: {
			const auto channel = [](editor::FieldValue &v, int i) -> f32 & {
				if (auto *p = std::get_if<Vector2>(&v)) {
					return i ? p->y : p->x;
				}
				if (auto *p = std::get_if<Vector3>(&v)) {
					return i == 0 ? p->x : i == 1 ? p->y : p->z;
				}
				auto &p = std::get<Color>(v);
				return i == 0 ? p.r : i == 1 ? p.g : i == 2 ? p.b : p.a;
			};
			int count = 4;
			if (shared.field.type == FieldType::Vector2) {
				count = 2;
			} else if (shared.field.type == FieldType::Vector3) {
				count = 3;
			}
			mask = 0;
			for (int i = 0; i < count; ++i) {
				if (i) {
					ImGui::SameLine();
				}
				ImGui::PushID(i);
				ImGui::SetNextItemWidth(std::max(45.f,
					(ImGui::GetContentRegionAvail().x - (count - i - 1) * 6.f) / (count - i)));
				f32 &v = channel(value, i);
				if (ImGui::DragFloat("##channel", &v,
						shared.field.type == FieldType::Color ? 0.01f : 1.f, 0, 0, "%.3f")) {
					changed = true;
					mask |= 1u << i;
				}
				status();
				ImGui::PopID();
			}
			break;
		}
		default: {
			std::visit(
				[&](auto &v) {
					using T = std::decay_t<decltype(v)>;
					if constexpr (std::is_same_v<T, i32> || std::is_same_v<T, u32> ||
						std::is_same_v<T, f32> || std::is_same_v<T, f64>) {
						ImGuiDataType kind = ImGuiDataType_Double;
						if constexpr (std::is_same_v<T, i32>) {
							kind = ImGuiDataType_S32;
						} else if constexpr (std::is_same_v<T, u32>) {
							kind = ImGuiDataType_U32;
						} else if constexpr (std::is_same_v<T, f32>) {
							kind = ImGuiDataType_Float;
						}
						changed = ImGui::DragScalar("##value", kind, &v, 1.f);
						status();
					}
				},
				value);
			break;
		}
		}
		ImGui::EndDisabled();
		if (editable && changed && shared.field.type == FieldType::ResourceRef) {
			finishBulkInspector();
			const auto edits = editor::AssignBulkValue(values, value);
			if (!editor::ExecuteBulkFieldEdit(_sceneSaveStates, _editingScene, edits, &resources,
					created) &&
				created.IsValid()) {
				_editingScene.Disown(resources, created);
			}
		} else if (editable && changed) {
			auto edits = editor::AssignBulkValue(values, value, mask);
			if (!_bulkPending || _bulkPending->edits.front().component != shared.component ||
				_bulkPending->edits.front().field != shared.field.name) {
				finishBulkInspector();
				_bulkPending = BulkPending{_editingScene, selection(), edits};
			} else {
				for (std::size_t i = 0; i < edits.size(); ++i) {
					_bulkPending->edits[i].after = edits[i].after;
				}
			}
			if (!editor::PreviewBulkFieldEdit(_bulkPending->edits)) {
				finishBulkInspector(true);
			}
		}
		if (finished) {
			finishBulkInspector();
		}
		ImGui::PopID();
		ImGui::PopID();
	}
}
} // namespace sowa
