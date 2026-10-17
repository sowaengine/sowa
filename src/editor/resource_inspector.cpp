#include "editor/resource_inspector.hpp"
#include "ui/theme.hpp"

#include <algorithm>
#include <cfloat>
#include <imgui.h>
#include <misc/cpp/imgui_stdlib.h>

#include "core/filesystem/file_system.hpp"
#include "core/reflection/field_ref.hpp"
#include "editor/asset_picker.hpp"
#include "editor/filesystem_browser.hpp"
#include "editor/icons.hpp"
#include "editor/inspector.hpp"
#include "editor/scene_commands.hpp"
#include "editor/style.hpp"
#include "resource/image_texture.hpp"

namespace sowa::editor {

std::string SaveResource(Resource &resource) {
	auto archive = Archive::Save();
	resource.Serialize(archive);
	return archive.ToString();
}

std::string ResourceLabel(ResourceRegistry &resources, ResourceID id) {
	auto resource = resources.Get(id);
	if (!resource) {
		return id.IsValid() ? "Missing resource" : "Empty";
	}
	std::string label(resources.TypeNameOf(id));
	for (const auto &field : resource->Fields()) {
		if (field.name == "path" && field.type == FieldType::String) {
			const auto *path = reinterpret_cast<const std::string *>(
				reinterpret_cast<const u8 *>(resource.get()) + field.offset);
			if (!path->empty()) {
				return AssetName(*path);
			}
		}
	}
	return label;
}

std::vector<ResourceID> SceneResourcesOfType(Scene scene, ResourceRegistry &resources,
	std::string_view type) {
	std::vector<ResourceID> matches;
	for (ResourceID id : scene.OwnedResources()) {
		if (resources.Has(id) && (type.empty() || resources.TypeNameOf(id) == type)) {
			matches.push_back(id);
		}
	}
	return matches;
}

ResourceID CreateSceneResource(Scene scene, ResourceRegistry &resources, std::string_view type) {
	if (!scene.IsValid()) {
		return {};
	}
	auto resource = resources.Create(type);
	if (!resource) {
		return {};
	}
	if (!scene.Own(resources, resource->Id())) {
		resources.Remove(resource->Id());
		return {};
	}
	return resource->Id();
}

namespace {
AssetKind resourceFileKind(std::string_view type) {
	if (type == "ImageTexture") {
		return AssetKind::Image;
	}
	if (type == "AudioClip") {
		return AssetKind::Audio;
	}
	if (type == "Font") {
		return AssetKind::Font;
	}
	if (type == "PackedScene") {
		return AssetKind::Scene;
	}
	return AssetKind::Other;
}

bool editValue(FieldRef field, const char *label) {
	switch (field.Type()) {
	case FieldType::Bool:
		return ImGui::Checkbox(label, field.As<bool>());
	case FieldType::I32:
		return ImGui::DragInt("##value", field.As<i32>());
	case FieldType::U32:
		return ImGui::DragScalar("##value", ImGuiDataType_U32, field.Data());
	case FieldType::F32:
		return ImGui::DragFloat("##value", field.As<f32>(), 0.05f);
	case FieldType::F64:
		return ImGui::DragScalar("##value", ImGuiDataType_Double, field.Data(), 0.05f);
	case FieldType::Vector2:
		return ImGui::DragFloat2("##value", &field.As<Vector2>()->x, 0.05f);
	case FieldType::Vector3:
		return ImGui::DragFloat3("##value", &field.As<Vector3>()->x, 0.05f);
	case FieldType::Color:
		return ImGui::ColorEdit4("##value", &field.As<Color>()->r, ImGuiColorEditFlags_NoInputs);
	case FieldType::String: {
		return ImGui::InputText("##value", field.As<std::string>());
	}
	case FieldType::Enum: {
		auto &value = *static_cast<i32 *>(field.Data());
		const auto *name = field.Info()->FindEnumName(value);
		bool changed = false;
		if (ImGui::BeginCombo("##value", name ? name->c_str() : "")) {
			for (const auto &constant : field.Info()->enumConstants) {
				if (ImGui::Selectable(constant.name.c_str(), value == constant.value)) {
					value = constant.value;
					changed = true;
				}
			}
			ImGui::EndCombo();
		}
		return changed;
	}
	default:
		return false;
	}
}
} // namespace

ResourceSelection ResourceInspector::Draw(const char *id, ResourceID value, std::string_view type,
	Scene scene, ResourceRegistry &resources, FileSystem *files,
	std::vector<SceneDocumentSaveState> &documents, bool editable) {
	ResourceSelection result{value};
	ImGui::PushID(id);
	ImGui::BeginDisabled(!editable);
	ImGui::SetNextItemWidth(-FLT_MIN);
	ImGui::SetNextWindowSizeConstraints(ImVec2(100, 0), ImVec2(360, 320));
	const auto label = ResourceLabel(resources, value);
	if (const MenuStyle menuStyle; ImGui::BeginCombo("##resource", label.c_str())) {
		if (ImGui::Selectable("Clear", !value.IsValid())) {
			result = {{}, true, false};
		}
		for (const auto &name : resources.TypeNames()) {
			if (!type.empty() && type != name) {
				continue;
			}
			if (ImGui::Selectable(("New " + name).c_str())) {
				const auto created = CreateSceneResource(scene, resources, name);
				if (created.IsValid()) {
					result = {created, true, true};
				}
			}
		}
		ImGui::SeparatorText("In this scene");
		const auto existing = SceneResourcesOfType(scene, resources, type);
		for (size_t index = 0; index < existing.size(); ++index) {
			const ResourceID candidate = existing[index];
			ImGui::PushID(static_cast<int>(index));
			// A small ordinal distinguishes multiple unfiled resources of one type.
			const auto name =
				ResourceLabel(resources, candidate) + " (" + std::to_string(index + 1) + ")";
			if (ImGui::Selectable(name.c_str(), candidate == value)) {
				result = {candidate, candidate != value, false};
			}
			ImGui::PopID();
		}
		if (existing.empty()) {
			ImGui::TextDisabled("No matching resources");
		}
		ImGui::EndCombo();
	}
	ImGui::EndDisabled();

	if (type == "SpriteSheetAnimation" ||
		resources.TypeNameOf(result.value) == "SpriteSheetAnimation") {
		result.editRequested = ImGui::Button("Edit", {-1, 0});
		ImGui::PopID();
		return result;
	}

	if (result.created) {
		ImGui::SetNextItemOpen(true);
	}
	auto resource = resources.Get(result.value);
	const bool cycle =
		std::find(_ancestors.begin(), _ancestors.end(), result.value) != _ancestors.end();
	if (resource && !cycle && _ancestors.size() < 8) {
		const std::string heading = std::string(ICON_FA_DATABASE "  ") +
									std::string(resources.TypeNameOf(result.value)) +
									"###Properties";
		ImGui::PushStyleColor(ImGuiCol_Header, ImGui::GetStyleColorVec4(ImGuiCol_FrameBg));
		ImGui::PushStyleColor(ImGuiCol_HeaderHovered,
			ImGui::GetStyleColorVec4(ImGuiCol_FrameBgHovered));
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10.f, 6.f));
		const bool expanded = ImGui::CollapsingHeader(heading.c_str());
		ImGui::PopStyleVar();
		ImGui::PopStyleColor(2);
		if (expanded) {
			ImGui::PushStyleColor(ImGuiCol_ChildBg, ImGui::GetStyleColorVec4(ImGuiCol_FrameBg));
			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10.f, 10.f));
			const bool visible = ImGui::BeginChild("##resource_properties", ImVec2(0.f, 0.f),
				ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_AlwaysAutoResize |
					ImGuiChildFlags_AlwaysUseWindowPadding,
				ImGuiWindowFlags_NoScrollbar);
			if (visible) {
				_ancestors.push_back(result.value);
				drawProperties(*resource, scene, resources, files, documents,
					editable && scene.Owns(result.value));
				_ancestors.pop_back();
			}
			ImGui::EndChild();
			ImGui::PopStyleVar();
			ImGui::PopStyleColor();
		}
	}
	ImGui::PopID();
	return result;
}

void ResourceInspector::drawProperties(Resource &resource, Scene scene, ResourceRegistry &resources,
	FileSystem *files, std::vector<SceneDocumentSaveState> &documents, bool editable) {
	ImGui::BeginDisabled(!editable);
	if (auto texture = dynamic_cast<ImageTexture *>(&resource); texture && texture->IsLoaded()) {
		if (auto *device = resources.Context().device) {
			const float width = std::clamp(ImGui::GetContentRegionAvail().x, 1.f, 96.f);
			const float aspect = texture->Size().y / std::max(1.f, texture->Size().x);
			const auto native = device->GetNativeTextureId(texture->Handle());
			// Match the viewport's Y-up texture coordinates in ImGui's Y-down layout.
			ImGui::Image(static_cast<ImTextureID>(native),
				ImVec2(width, width * std::min(aspect, 2.f)), ImVec2(0, 1), ImVec2(1, 0));
		}
	}
	if (resource.Fields().empty()) {
		if (resources.TypeNameOf(resource.Id()) == "AnimationLibrary") {
			ImGui::TextWrapped("Edit tracks in the Animation panel.");
		} else if (resources.TypeNameOf(resource.Id()) == "SpriteSheetAnimation") {
			ImGui::TextWrapped("Edit textures and frames in the Sprite Sheet panel.");
		} else {
			ImGui::TextDisabled("No exposed properties");
		}
	}
	for (const auto &info : resource.Fields()) {
		if (!info.IsExposed()) {
			continue;
		}
		ImGui::PushID(info.name.c_str());
		const std::string fieldLabel = DisplayName(info.name);
		if (info.type != FieldType::Bool) {
			ImGui::TextWrapped("%s", fieldLabel.c_str());
		}
		const bool filePath = files && info.type == FieldType::String && info.name == "path";
		ImGui::SetNextItemWidth(filePath ? std::max(1.f, ImGui::GetContentRegionAvail().x - 32.f -
															 ImGui::GetStyle().ItemSpacing.x)
										 : -FLT_MIN);
		FieldRef field(reinterpret_cast<u8 *>(&resource) + info.offset, &info);
		const auto before = SaveResource(resource);
		if (info.type == FieldType::ResourceRef) {
			auto &reference = *field.As<ResourceID>();
			const auto selection = Draw("nested", reference, info.resourceType, scene, resources,
				files, documents, editable);
			if (selection.changed) {
				reference = selection.value;
				RecordResourceEdit(documents, scene, resources, resource.Id(), before,
					SaveResource(resource), selection.created ? selection.value : ResourceID{});
				resources.Reload(resource.Id());
			}
		} else {
			const auto widget = ImGui::GetID(info.type == FieldType::Bool ? "###value" : "##value");
			const bool changed = editValue(field, (fieldLabel + "###value").c_str());
			if (ImGui::IsItemActivated()) {
				Commit(resources, documents);
				_activeWidget = widget;
				_pendingResource = resource.Id();
				_pendingScene = scene;
				_beforeEdit = before;
			}
			const bool immediate = info.type == FieldType::Enum || info.type == FieldType::Bool;
			if ((immediate && changed) || ImGui::IsItemDeactivatedAfterEdit()) {
				if (_activeWidget == widget) {
					Commit(resources, documents);
				} else {
					RecordResourceEdit(documents, scene, resources, resource.Id(), before,
						SaveResource(resource));
					resources.Reload(resource.Id());
				}
			}
		}
		if (filePath) {
			ImGui::SameLine();
			if (ImGui::Button(ICON_FA_FOLDER_OPEN "##browse", ImVec2(32.f, 0.f))) {
				ImGui::OpenPopup("##resource_file_picker");
			}
			if (ImGui::IsItemHovered()) {
				ImGui::SetTooltip("Browse project files");
			}
			if (const DialogStyle dialogStyle; BeginAssetPickerPopup("##resource_file_picker")) {
				auto &path = *field.As<std::string>();
				const auto picker =
					DrawAssetPicker(files, resourceFileKind(resources.TypeNameOf(resource.Id())),
						"Choose a resource file", {}, path);
				if (picker.path && *picker.path != path) {
					path = *picker.path;
					RecordResourceEdit(documents, scene, resources, resource.Id(), before,
						SaveResource(resource));
					resources.Reload(resource.Id());
				}
				if (picker.path || picker.cancelled) {
					ImGui::CloseCurrentPopup();
				}
				ImGui::EndPopup();
			}
		}
		ImGui::PopID();
	}
	if (auto *theme = dynamic_cast<UITheme *>(&resource)) {
		ImGui::SeparatorText("Control roles and variants");
		ImGui::TextWrapped("Each rule replaces the appearance for a role and optional variant. "
						   "Assign a variant through UIStyle on a control or subtree.");
		int remove = -1;
		for (size_t index = 0; index < theme->rules.size(); ++index) {
			ImGui::PushID(static_cast<int>(index));
			auto &rule = theme->rules[index];
			const std::string heading =
				rule.role + (rule.variant.empty() ? "" : " / " + rule.variant);
			if (ImGui::TreeNode("rule", "%s", heading.c_str())) {
				const auto before = SaveResource(resource);
				auto watch = [&]() {
					if (ImGui::IsItemActivated()) {
						Commit(resources, documents);
						_pendingResource = resource.Id();
						_pendingScene = scene;
						_beforeEdit = before;
						_activeWidget = ImGui::GetItemID();
					}
					if (ImGui::IsItemDeactivatedAfterEdit()) {
						Commit(resources, documents);
					}
				};
				ImGui::InputText("Role", &rule.role);
				watch();
				ImGui::InputText("Variant", &rule.variant);
				watch();
				for (auto [label, color] :
					{std::pair{"Background", &rule.appearance.background},
						{"Hovered", &rule.appearance.hovered},
						{"Pressed", &rule.appearance.pressed},
						{"Disabled", &rule.appearance.disabled}, {"Text", &rule.appearance.text},
						{"Accent", &rule.appearance.accent}, {"Border", &rule.appearance.border}}) {
					ImGui::ColorEdit4(label, &color->r);
					watch();
				}
				ImGui::DragFloat("Font size", &rule.appearance.fontSize, .5f, 1.f, 200.f);
				watch();
				ImGui::DragFloat2("Padding", &rule.appearance.padding.x, .5f, 0.f, 200.f);
				watch();
				ImGui::DragFloat("Border width", &rule.appearance.borderWidth, .1f, 0.f, 20.f);
				watch();
				for (auto [label, id] : {std::pair{"Font", &rule.appearance.font},
						 {"Texture", &rule.appearance.texture}}) {
					ImGui::TextUnformatted(label);
					const auto assignment = SaveResource(resource);
					const auto selection = Draw(label, *id,
						std::string_view(label) == "Font" ? "Font" : "ImageTexture", scene,
						resources, files, documents, editable);
					if (selection.changed) {
						*id = selection.value;
						RecordResourceEdit(documents, scene, resources, resource.Id(), assignment,
							SaveResource(resource),
							selection.created ? selection.value : ResourceID{});
					}
				}
				int imageMode = static_cast<int>(rule.appearance.imageMode);
				if (ImGui::Combo("Image mode", &imageMode, "Stretch\0Native\0Tile\0NineSlice\0")) {
					rule.appearance.imageMode = static_cast<UIImageMode>(imageMode);
				}
				watch();
				ImGui::Checkbox("Tile center", &rule.appearance.tileCenter);
				watch();
				ImGui::Checkbox("Tile edges", &rule.appearance.tileEdges);
				watch();
				ImGui::DragFloat2("Region position", &rule.appearance.regionPosition.x);
				watch();
				ImGui::DragFloat2("Region size", &rule.appearance.regionSize.x);
				watch();
				ImGui::DragFloat2("Left / top borders", &rule.appearance.borderMin.x);
				watch();
				ImGui::DragFloat2("Right / bottom borders", &rule.appearance.borderMax.x);
				watch();
				if (ImGui::Button("Remove rule")) {
					remove = static_cast<int>(index);
				}
				ImGui::TreePop();
			}
			ImGui::PopID();
		}
		if (remove >= 0) {
			Commit(resources, documents);
			const auto before = SaveResource(resource);
			theme->rules.erase(theme->rules.begin() + remove);
			RecordResourceEdit(documents, scene, resources, resource.Id(), before,
				SaveResource(resource));
		}
		if (ImGui::Button("Add role / variant")) {
			Commit(resources, documents);
			const auto before = SaveResource(resource);
			theme->rules.push_back({"Button", "Custom", *theme});
			RecordResourceEdit(documents, scene, resources, resource.Id(), before,
				SaveResource(resource));
		}
	}
	ImGui::EndDisabled();
}

void ResourceInspector::Commit(ResourceRegistry &resources,
	std::vector<SceneDocumentSaveState> &documents) {
	if (auto resource = resources.Get(_pendingResource)) {
		const auto after = SaveResource(*resource);
		if (after != _beforeEdit) {
			RecordResourceEdit(documents, _pendingScene, resources, _pendingResource, _beforeEdit,
				after);
			resources.Reload(_pendingResource);
		}
	}
	_pendingResource = {};
	_pendingScene = {};
	_activeWidget = 0;
	_beforeEdit.clear();
}

} // namespace sowa::editor
