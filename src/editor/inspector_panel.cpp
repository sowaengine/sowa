#include "core/filesystem/file_system.hpp"
#include "core/log.hpp"
#include "ecs/world.hpp"
#include "editor/asset_picker.hpp"
#include "editor/editor.hpp"
#include "editor/icons.hpp"
#include "editor/inspector.hpp"
#include "editor/style.hpp"
#include "editor/widgets.hpp"
#include "imgui_internal.h"
#include "resource/resource_registry.hpp"
#include "scene/scene_manager.hpp"
#include "script/script_server.hpp"
#include "ui/components.hpp"
#include <imgui.h>
#include <misc/cpp/imgui_stdlib.h>

#include <algorithm>
#include <initializer_list>
#include <utility>
#include <variant>

namespace sowa {

bool Editor::drawComponentHeader(const char *typeName, const char *label, const char *icon,
	bool playing) {
	ImGui::PushID(typeName);
	const auto type = ComponentType(_selected.Raw().world().lookup(typeName));
	const bool removable = editor::CanRemoveComponent(_selected, type, playing);
	ImGuiWindow *window = ImGui::GetCurrentWindow();
	const float right = window->WorkRect.Max.x;
	if (removable) {
		window->WorkRect.Max.x -= ImGui::GetFrameHeight() + ImGui::GetStyle().ItemInnerSpacing.x;
	}
	const bool expanded = editor::SectionHeader(label, icon);
	window->WorkRect.Max.x = right;
	const ImVec2 rowMin = ImGui::GetItemRectMin(), rowMax = ImGui::GetItemRectMax();
	if (const editor::MenuStyle style; ImGui::BeginPopupContextItem("component_actions")) {
		if (ImGui::MenuItem("Remove component", nullptr, false, removable)) {
			_removeComponentRequested = typeName;
		}
		ImGui::EndPopup();
	}
	if (removable &&
		editor::TreeRowButton("remove", ICON_FA_TRASH, "Remove component", rowMin, rowMax)) {
		_removeComponentRequested = typeName;
	}
	ImGui::PopID();
	return expanded;
}

void Editor::drawInspector(World &world, SceneManager &scenes, ScriptServer &scripts,
	ProjectSettings &project, ResourceRegistry &resources, bool playing) {
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10.f, 10.f));
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8.f, 6.f));
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8.f, 4.f));
	ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(4.f, 3.f));
	ImGui::Begin(ICON_FA_SLIDERS "  Inspector");
	ImGui::PopStyleVar();

	if (!_selected.IsAlive()) {
		ImGui::Spacing();
		ImGui::TextColored(editor::theme::Accent, ICON_FA_SLIDERS "  Inspector");
		ImGui::Spacing();
		ImGui::TextWrapped("Select a node in the scene or viewport to edit its properties.");
		ImGui::PopStyleVar(3);
		ImGui::End();
		return;
	}

	const Entity entity = _selected;
	// Structure (components, scripts) is editable only while stopped; field
	// values are also editable on live entities while playing.
	const bool editable = editor::CanEdit(entity, playing);
	const bool fieldsEditable = editor::CanEditFields(entity, scenes);
	const auto appearance = editor::EntityAppearance(entity);
	ImGui::TextColored(appearance.color, "%s  %s", appearance.icon, appearance.label);
	if (playing) {
		ImGui::SameLine();
		ImGui::TextDisabled(fieldsEditable ? " / Live" : " / Playing");
	} else if (!editable) {
		ImGui::SameLine();
		ImGui::TextDisabled(" / Inherited");
	}
	ImGui::Separator();

	if (editor::BeginInspectorTable("##entity", editor::InspectorLabelWidth("Name", {}))) {
		ImGui::BeginDisabled(!fieldsEditable);
		editor::InspectorFieldRow("name", std::string("Name"));
		std::string name = entity.GetName();
		const std::string nameBefore = entity.GetName();
		if (ImGui::InputText("##", &name)) {
			_selected.SetName(name);
		}
		if (ImGui::IsItemActivated()) {
			_pendingRenameEntity = entity;
			_pendingRenameScene = _editingScene;
			_pendingRenameBefore = nameBefore;
		}
		if (ImGui::IsItemDeactivatedAfterEdit()) {
			const bool pendingMatches = _pendingRenameEntity == entity;
			const std::string before = pendingMatches ? _pendingRenameBefore : nameBefore;
			editor::RecordRename(_sceneSaveStates,
				pendingMatches ? _pendingRenameScene : _editingScene, entity, before,
				entity.GetName());
			_pendingRenameEntity = {};
			_pendingRenameScene = {};
			_pendingRenameBefore.clear();
		}
		ImGui::PopID();
		ImGui::EndDisabled();
		ImGui::EndTable();
	}

	ImGui::Dummy(ImVec2(0.f, 4.f));
	drawAddComponent(world, scripts, resources, playing);

	if (editor::SectionHeader("Scripts", ICON_FA_FILE_CODE, editor::theme::Green)) {
		ImGui::Indent(8.f);
		const std::vector<std::string> attached = scripts.AttachedScriptPaths(entity);
		std::string removePath;
		if (attached.empty()) {
			ImGui::TextDisabled("No scripts attached.");
		} else if (ImGui::BeginTable("##scripts", 2,
					   ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_BordersInnerH |
						   ImGuiTableFlags_RowBg)) {
			ImGui::TableSetupColumn("script", ImGuiTableColumnFlags_WidthStretch);
			ImGui::TableSetupColumn("remove", ImGuiTableColumnFlags_WidthFixed, 44.f);
			for (const std::string &path : attached) {
				const std::string name = editor::AssetName(path);
				ImGui::PushID(path.c_str());
				ImGui::TableNextRow();
				ImGui::TableNextColumn();
				ImGui::AlignTextToFramePadding();
				ImGui::Text("%s  %s", ICON_FA_FILE_CODE,
					name.empty() ? path.c_str() : name.c_str());
				if (ImGui::IsItemHovered()) {
					const std::vector<ComponentType> types = scripts.ScriptComponentTypes(path);
					ImGui::BeginTooltip();
					ImGui::TextUnformatted(path.c_str());
					for (ComponentType type : types) {
						ImGui::TextDisabled("Component: %s", type.Name().c_str());
					}
					ImGui::EndTooltip();
				}
				ImGui::TableNextColumn();
				ImGui::BeginDisabled(!editable);
				if (editor::IconActionButton(ICON_FA_TRASH, "Remove script")) {
					removePath = path;
				}
				ImGui::EndDisabled();
				ImGui::PopID();
			}
			ImGui::EndTable();
		}
		if (!removePath.empty()) {
			if (!editor::ExecuteScriptRemove(_sceneSaveStates, _editingScene, entity, scripts,
					removePath)) {
				_scriptInspectorError = "The script could not be removed.";
			} else {
				_scriptInspectorError.clear();
			}
		}

		if (!_scriptInspectorError.empty()) {
			ImGui::SameLine();
			ImGui::TextColored(ImVec4(0.95f, 0.38f, 0.34f, 1.f), "%s",
				_scriptInspectorError.c_str());
		}
		ImGui::Unindent(8.f);
	}

	if (const editor::DialogStyle dialogStyle; editor::BeginAssetPickerPopup("##script_picker")) {
		const auto picker = editor::DrawAssetPicker(_fileSystem, editor::AssetKind::Script,
			"Attach a script", _scriptInspectorError);
		if (const auto &path = picker.path; path && _fileSystem) {
			const std::vector<std::string> beforeAttach = scripts.AttachedScriptPaths(entity);
			const bool alreadyAttached =
				std::find(beforeAttach.begin(), beforeAttach.end(), *path) != beforeAttach.end();
			const editor::ScriptEditResult result = editor::TryAttachEntityScript(scripts,
				*_fileSystem, project, entity, *path, playing);
			_scriptInspectorError = result.error;
			if (result) {
				if (!alreadyAttached) {
					editor::RecordScriptAttach(_sceneSaveStates, _editingScene, entity, scripts,
						result.path);
				}
				ImGui::CloseCurrentPopup();
			}
		}
		if (picker.cancelled) {
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}

	drawTilemapInspector(resources, playing);
	drawAnimationInspector(resources, playing);
	for (ComponentType type : entity.GetComponents()) {
		if (type.Name() == "Tilemap" || type.Name() == "AnimationPlayer") {
			continue;
		}
		if (type.Name() == "Particles2D") {
			drawParticlesInspector(world, resources, playing, fieldsEditable);
			continue;
		}
		const std::vector<const FieldInfo *> fields = editor::ExposedFields(type);
		if (type.Name() == "Hidden") {
			continue;
		}

		const std::string typeName = type.Name();
		ImGui::PushID(reinterpret_cast<void *>(static_cast<uintptr_t>(type.Raw().id())));
		if (drawComponentHeader(typeName.c_str(), typeName.c_str(), ICON_FA_CUBE, playing)) {
			ImGui::Indent(8.f);
			ImGui::BeginDisabled(!fieldsEditable);
			if (typeName == "NinePatchRect" || typeName == "UINinePatchRect") {
				if (ImGui::Button("Edit padding visually...")) {
					finishNinePatchPadding();
					_ninePatchType = typeName;
					_showNinePatchEditor = true;
				}
			}
			if (typeName == "UIRect") {
				if (auto *rect = _selected.Get<UIRect>()) {
					ImGui::TextDisabled("Resolved: %.0f x %.0f at %.0f, %.0f", rect->resolved.w,
						rect->resolved.h, rect->resolved.x, rect->resolved.y);
					if (ImGui::BeginCombo("Anchor preset", "Choose...")) {
						for (const char *preset : {"TopLeft", "Center", "TopRight", "BottomLeft",
								 "BottomRight", "Stretch"}) {
							if (ImGui::Selectable(preset)) {
								const UIRect before = *rect;
								SetUIAnchorPreset(*rect, preset);
								editor::RecordUIRectEdit(_sceneSaveStates, _editingScene, entity,
									before, *rect);
							}
						}
						ImGui::EndCombo();
					}
				}
			}
			const float labelWidth = editor::InspectorLabelWidth("", fields);
			for (const FieldInfo *field : fields) {
				if (field != nullptr) {
					drawInspectorField(entity, type, *field, fieldsEditable, world, resources,
						labelWidth);
				}
			}
			ImGui::EndDisabled();
			ImGui::Unindent(8.f);
		}
		ImGui::PopID();
	}

	if (!_removeComponentRequested.empty()) {
		commitPendingSceneEdits(resources);
		StopAnimationPreview();
		editor::ExecuteRemoveComponent(_sceneSaveStates, _editingScene, entity,
			world.FindComponent(_removeComponentRequested));
		_removeComponentRequested.clear();
	}

	ImGui::PopStyleVar(3);
	ImGui::End();
}

void Editor::drawInspectorField(Entity entity, ComponentType type, const FieldInfo &field,
	bool editable, World &world, ResourceRegistry &resources, float labelWidth) {
	FieldRef ref = entity.GetField(type, field.name);
	if (!ref) {
		return;
	}
	editor::FieldValue before;
	if (!editor::CaptureFieldValue(entity, type, field, before)) {
		return;
	}

	// Resources use the full panel width, including their expanded properties.
	// Ordinary values retain the compact label/value layout.
	const bool resourceField = field.type == FieldType::ResourceRef;
	if (resourceField) {
		ImGui::PushID(field.name.c_str());
		ImGui::TextUnformatted(editor::DisplayName(field.name).c_str());
	} else {
		if (!editor::BeginInspectorTable(field.name.c_str(), labelWidth)) {
			return;
		}
		editor::InspectorFieldRow(field.name.c_str(), editor::DisplayName(field.name));
	}
	if (type.Name() == "AnimatedSprite" && field.name == "currentAnimation") {
		auto *sprite = entity.Get<AnimatedSprite>();
		auto library = resources.Get<SpriteSheetAnimation>(sprite->animations);
		if (editor::DrawSpriteAnimationPicker("##animation", library.get(),
				sprite->currentAnimation)) {
			sprite->frame = 0;
			sprite->playback = {};
			editor::RecordFieldEdit(_sceneSaveStates, _editingScene, entity, type, field, before,
				sprite->currentAnimation);
		}
		ImGui::PopID();
		ImGui::EndTable();
		return;
	}

	if (field.type == FieldType::U32 &&
		(type.Name() == "Collider2D" || type.Name() == "Raycast2D") &&
		(field.name == "collisionLayer" || field.name == "collisionMask")) {
		auto &value = *ref.As<u32>();
		const std::string label = fmt::format("0x{:08X}##mask", value);
		if (ImGui::Button(label.c_str())) {
			ImGui::OpenPopup("Layers");
		}
		bool changed = false;
		if (ImGui::BeginPopup("Layers")) {
			if (ImGui::Button("All")) {
				value = 0xffffffffu;
				changed = true;
			}
			ImGui::SameLine();
			if (ImGui::Button("None")) {
				value = 0;
				changed = true;
			}
			if (ImGui::BeginTable("LayerBits", 4)) {
				for (u32 layer = 1; layer <= 32; ++layer) {
					ImGui::TableNextColumn();
					const u32 bit = u32{1} << (layer - 1);
					bool checked = (value & bit) != 0;
					if (ImGui::Checkbox(std::to_string(layer).c_str(), &checked)) {
						value = checked ? value | bit : value & ~bit;
						changed = true;
					}
				}
				ImGui::EndTable();
			}
			ImGui::EndPopup();
		}
		if (changed) {
			editor::RecordFieldEdit(_sceneSaveStates, _editingScene, entity, type, field, before,
				editor::FieldValue(value));
		}
		ImGui::PopID();
		ImGui::EndTable();
		return;
	}

	switch (field.type) {
	case FieldType::Bool:
		if (bool *value = ref.As<bool>()) {
			ImGui::Checkbox("##", value);
		}
		break;
	case FieldType::I32:
		if (i32 *value = ref.As<i32>()) {
			ImGui::DragInt("##", value);
		}
		break;
	case FieldType::U32:
		if (u32 *value = ref.As<u32>()) {
			ImGui::DragScalar("##", ImGuiDataType_U32, value, 1.f);
		}
		break;
	case FieldType::F32:
		if (f32 *value = ref.As<f32>()) {
			ImGui::DragFloat("##", value);
		}
		break;
	case FieldType::F64:
		if (f64 *value = ref.As<f64>()) {
			ImGui::DragScalar("##", ImGuiDataType_Double, value, 1.f);
		}
		break;
	case FieldType::String:
		if (std::string *value = ref.As<std::string>()) {
			if ((type.Name() == "Text2D" || type.Name().starts_with("UI")) &&
				(field.name == "text" || field.name == "items" || field.name == "titles")) {
				ImGui::InputTextMultiline("##", value,
					ImVec2(-1.f, ImGui::GetTextLineHeight() * 5.f));
				break;
			}
			ImGui::InputText("##", value);
		}
		break;
	case FieldType::EntityRef:
		if (Entity *target = ref.As<Entity>()) {
			static std::string editBuffer;
			static ImGuiID editing = 0;
			const ImGuiID id = ImGui::GetID("##");
			const std::string path = target->IsAlive() ? target->GetPath() : std::string();
			std::string local = path;
			std::string &buffer = editing == id ? editBuffer : local;
			ImGui::InputText("##", &buffer);
			if (ImGui::IsItemActivated()) {
				editBuffer = buffer;
				editing = id;
			}
			if (ImGui::IsItemDeactivatedAfterEdit()) {
				if (editable) {
					if (editBuffer.empty()) {
						*target = Entity();
					} else {
						*target = world.Find(editBuffer);
					}
				}
				editing = 0;
			}
		}
		break;
	case FieldType::Vector2:
		if (Vector2 *value = ref.As<Vector2>()) {
			ImGui::DragFloat2("##", &value->x);
		}
		break;
	case FieldType::Vector3:
		if (Vector3 *value = ref.As<Vector3>()) {
			ImGui::DragFloat3("##", &value->x);
		}
		break;
	case FieldType::Color:
		if (Color *value = ref.As<Color>()) {
			ImGui::ColorEdit4("##", &value->r);
		}
		break;
	case FieldType::Enum: {
		i32 *value = static_cast<i32 *>(ref.Data());
		if (value == nullptr) {
			break;
		}
		const std::string *current = field.FindEnumName(*value);
		const std::string preview =
			current != nullptr ? editor::DisplayName(*current) : std::string();
		if (ImGui::BeginCombo("##", preview.c_str())) {
			for (const EnumConstant &constant : field.enumConstants) {
				const bool selected = constant.value == *value;
				const std::string item = editor::DisplayName(constant.name) + "##" + constant.name;
				if (ImGui::Selectable(item.c_str(), selected)) {
					*value = constant.value;
					editor::RecordFieldEdit(_sceneSaveStates, _editingScene, entity, type, field,
						before, editor::FieldValue(constant.value));
				}
				if (selected) {
					ImGui::SetItemDefaultFocus();
				}
			}
			ImGui::EndCombo();
		}
		break;
	}
	case FieldType::ResourceRef: {
		auto &value = *ref.As<ResourceID>();
		// Resources need an editor document to own and record them, so they
		// stay read-only while playing (no document is edited then).
		const auto selection =
			_resourceInspector.Draw("resource", value, field.resourceType, _editingScene, resources,
				_fileSystem, _sceneSaveStates, editable && _editingScene.IsValid());
		if (selection.editRequested) {
			_showSpriteSheetEditor = true;
			ImGui::SetWindowFocus("Sprite Sheet");
		}
		if (selection.changed) {
			const auto previous = std::get<ResourceID>(before);
			entity.GetField(type, field.name).Set(selection.value);
			editor::RecordResourceAssignment(_sceneSaveStates, _editingScene, entity, type, field,
				resources, previous, selection.value, selection.created);
		}
		break;
	}
	}
	if (field.type != FieldType::Enum && field.type != FieldType::ResourceRef) {
		finishInspectorFieldEdit(entity, type, field, before);
	}
	ImGui::PopID();
	if (!resourceField) {
		ImGui::EndTable();
	}
}

void Editor::finishInspectorFieldEdit(Entity entity, ComponentType type, const FieldInfo &field,
	const editor::FieldValue &before) {
	if (ImGui::IsItemActivated()) {
		_pendingFieldEdit = PendingFieldEdit{_editingScene, entity, type, field.name, before};
	}
	if (!ImGui::IsItemDeactivatedAfterEdit()) {
		return;
	}

	editor::FieldValue after;
	if (!editor::CaptureFieldValue(entity, type, field, after)) {
		_pendingFieldEdit.reset();
		return;
	}
	editor::FieldValue original = before;
	if (_pendingFieldEdit && _pendingFieldEdit->entity == entity &&
		_pendingFieldEdit->type == type && _pendingFieldEdit->field == field.name) {
		original = _pendingFieldEdit->before;
	}
	const Scene editScene = _pendingFieldEdit ? _pendingFieldEdit->scene : _editingScene;
	editor::RecordFieldEdit(_sceneSaveStates, editScene, entity, type, field, std::move(original),
		std::move(after));
	_pendingFieldEdit.reset();
}

void Editor::drawParticlesInspector(World &world, ResourceRegistry &resources, bool playing,
	bool editable) {
	auto *emitter = _selected.Get<Particles2D>();
	if (!emitter ||
		!drawComponentHeader("Particles2D", "Particles 2D", ICON_FA_WAND_MAGIC_SPARKLES, playing)) {
		return;
	}

	const auto type = world.FindComponent("Particles2D");
	ImGui::PushID("particles2d");
	ImGui::BeginDisabled(!editable);

	bool emitting = emitter->IsEmitting();
	const bool previous = emitter->isEmitting;
	if (ImGui::Checkbox("Emitting", &emitting)) {
		emitter->SetEmitting(emitting);
		const auto *field = type.FindField("is_emitting");
		editor::RecordFieldEdit(_sceneSaveStates, _editingScene, _selected, type, *field, previous,
			emitting);
	}

	ImGui::SetNextItemWidth(-FLT_MIN);
	if (ImGui::BeginCombo("##preset", "Preset")) {
		const char *names[] = {"Smoke", "Flame", "Water", "Sparks", "Explosion", "Snow"};
		for (int index = 0; index < 6; ++index) {
			if (ImGui::Selectable(names[index])) {
				const auto before = *emitter;
				*emitter = MakeParticlePreset(static_cast<ParticlePreset>(index));
				editor::RecordParticlesEdit(_sceneSaveStates, _editingScene, _selected, before,
					*emitter);
			}
		}
		ImGui::EndCombo();
	}
	ImGui::EndDisabled();

	auto fields = [&](const char *title, std::initializer_list<const char *> names) {
		if (!ImGui::TreeNode(title)) {
			return;
		}
		ImGui::BeginDisabled(!editable);
		for (const char *name : names) {
			const auto *field = type.FindField(name);
			if (field) {
				drawInspectorField(_selected, type, *field, editable, world, resources);
			}
		}
		ImGui::EndDisabled();
		ImGui::TreePop();
	};

	fields("Emission",
		{"amount", "emissionRate", "burstCount", "lifetime", "lifetimeRandomness", "oneShot",
			"duration", "startDelay", "preprocess", "speedScale", "seed", "localCoords"});
	fields("Shape", {"emissionShape", "emissionSize", "emissionRadius", "emissionInnerRadius"});
	fields("Motion",
		{"direction", "spread", "initialVelocity", "inheritVelocity", "gravity",
			"linearAcceleration", "radialAcceleration", "tangentialAcceleration", "damping",
			"turbulenceStrength", "turbulenceFrequency", "turbulenceSpeed", "initialRotation",
			"angularVelocity", "alignToVelocity", "velocityStretch"});
	fields("Size", {"initialSize", "sizeStart", "sizeMiddle", "sizeEnd", "sizeMidpoint"});
	fields("Color", {"colorStart", "colorMiddle", "colorEnd", "colorMidpoint"});
	fields("Drawing", {"drawShape", "blendMode", "texture", "hFrames", "vFrames", "animationSpeed",
						  "animationLoop", "randomFrame"});
	fields("Collision", {"collisionEnabled", "collisionY", "collisionBounce", "collisionFriction"});
	ImGui::PopID();
}

} // namespace sowa
