#include "core/commands.hpp"
#include "editor/editor.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <imgui.h>
#include <misc/cpp/imgui_stdlib.h>

#include "ecs/world.hpp"
#include "editor/icons.hpp"
#include "editor/inspector.hpp"
#include "editor/style.hpp"
#include "resource/resource_registry.hpp"
#include "script/script_server.hpp"

namespace sowa {
namespace {
std::string saveAnimation(AnimationLibrary &library) {
	auto archive = Archive::Save();
	library.Serialize(archive);
	return archive.ToString();
}

bool animationPicker(AnimationPlayer &player, const AnimationLibrary &library,
	const char *fieldLabel = "Current animation") {
	bool changed = false;
	const char *label =
		player.currentAnimation.empty() ? "Choose animation" : player.currentAnimation.c_str();
	if (ImGui::BeginCombo(fieldLabel, label)) {
		for (const auto &clip : library.clips) {
			if (ImGui::Selectable(clip.name.c_str(), player.currentAnimation == clip.name)) {
				player.currentAnimation = clip.name;
				changed = true;
			}
		}
		ImGui::EndCombo();
	}
	return changed;
}
} // namespace

void Editor::ApplyAnimationPreview(World &world, ResourceRegistry &resources, f64 delta,
	bool playing, Vector2 viewportSize) {
	if (!_initialized) {
		return;
	}
	if (playing) {
		_animationPreview.Stop();
		return;
	}
	_animationPreview.Select(Selected());
	_animationPreview.Apply(world, resources, delta, viewportSize);
}

void Editor::createAnimationLibrary(ResourceRegistry &resources) {
	std::optional<AnimationPlayer> before;
	if (auto *player = Selected().Get<AnimationPlayer>()) {
		before = *player;
	}
	auto library = resources.Create<AnimationLibrary>();
	if (!library || !_editingScene.Own(resources, library->Id())) {
		return;
	}

	const auto empty = saveAnimation(*library);
	AnimationClip clip;
	clip.name = "Animation";
	library->clips.push_back(clip);
	AnimationPlayer player = before.value_or(AnimationPlayer{});
	player.isPlaying = false;
	player.playback = {};
	player.library = library->Id();
	player.currentAnimation = clip.name;
	Selected().Add(player);
	editor::RecordAnimationEdit(_sceneSaveStates, _editingScene, Selected(), resources,
		library->Id(), before, player, empty, saveAnimation(*library), true);
	_animationTimeline.Reset();
	_showAnimationEditor = true;
	ImGui::SetWindowFocus("Animation");
}

void Editor::drawAddComponent(World &world, ScriptServer &scripts, ResourceRegistry &resources,
	bool playing) {
	ImGui::BeginDisabled(!editor::CanEdit(Selected(), playing));
	if (ImGui::Button(ICON_FA_PLUS "  Add component", {-1, 28})) {
		_componentSearch[0] = '\0';
		ImGui::OpenPopup("add_component");
	}
	bool addScript = false;
	if (const editor::MenuStyle menuStyle; ImGui::BeginPopup("add_component")) {
		if (ImGui::IsWindowAppearing()) {
			ImGui::SetKeyboardFocusHere();
		}
		ImGui::InputTextWithHint("##search", "Search components", _componentSearch,
			sizeof(_componentSearch));
		ImGui::Separator();
		auto types = editor::AddableComponents(world, Selected());
		std::sort(types.begin(), types.end(), [](auto a, auto b) { return a.Name() < b.Name(); });
		const std::string search = _componentSearch;
		bool found = false;
		for (const char *category : {"General", "Rendering", "Animation", "Physics", "Audio",
				 "User Interface", "Gameplay and Scripts"}) {
			std::vector<ComponentType> matches;
			for (auto type : types) {
				if (editor::ComponentCategory(type) == category &&
					MatchesCommand(type.Name() + " " + category, search)) {
					matches.push_back(type);
				}
			}
			if (matches.empty()) {
				continue;
			}
			found = true;
			const bool searching = !search.empty();
			if (searching) {
				ImGui::SeparatorText(category);
			}
			if (searching || ImGui::BeginMenu(category)) {
				for (auto type : matches) {
					if (ImGui::MenuItem(type.Name().c_str())) {
						commitPendingSceneEdits(resources);
						if (type.Name() == "AnimationPlayer") {
							createAnimationLibrary(resources);
						} else {
							editor::ExecuteAddComponent(_sceneSaveStates, _editingScene, Selected(),
								type, scripts);
							if (type.Name() == "AnimatedSprite") {
								_showSpriteSheetEditor = true;
							}
						}
					}
				}
				if (!searching) {
					ImGui::EndMenu();
				}
			}
		}
		if (!found) {
			ImGui::TextDisabled("No matching components");
		}
		ImGui::Separator();
		addScript = ImGui::MenuItem("Attach script...");
		ImGui::EndPopup();
	}
	if (addScript) {
		_scriptInspectorError.clear();
		ImGui::OpenPopup("##script_picker");
	}
	ImGui::EndDisabled();
}

void Editor::drawAnimationInspector(ResourceRegistry &resources, bool playing) {
	auto *player = Selected().Get<AnimationPlayer>();
	if (!player ||
		!drawComponentHeader("AnimationPlayer", "Animation Player", ICON_FA_FILM, playing)) {
		return;
	}
	ImGui::PushID("animation_player");
	const auto librarySelection =
		_resourceInspector.Draw("library", player->library, "AnimationLibrary", _editingScene,
			resources, _fileSystem, _sceneSaveStates, editor::CanEdit(Selected(), playing));
	player = Selected().Get<AnimationPlayer>();
	if (librarySelection.changed) {
		const auto previous = player->library;
		player->library = librarySelection.value;
		const auto type = Selected().Raw().world().lookup("AnimationPlayer");
		const auto component = ComponentType(type);
		const auto *field = component.FindField("library");
		editor::RecordResourceAssignment(_sceneSaveStates, _editingScene, Selected(), component,
			*field, resources, previous, player->library, librarySelection.created);
		player = Selected().Get<AnimationPlayer>();
	}
	auto library = resources.Get<AnimationLibrary>(player->library);
	if (!library) {
		ImGui::BeginDisabled(!editor::CanEdit(Selected(), playing));
		if (ImGui::Button("Create animation")) {
			createAnimationLibrary(resources);
		}
		ImGui::EndDisabled();
		ImGui::PopID();
		return;
	}
	auto before = *player;
	before.isPlaying = false;
	ImGui::BeginDisabled(!editor::CanEdit(Selected(), playing));
	const bool selected = animationPicker(*player, *library);
	bool changed = selected;
	changed |= ImGui::Checkbox("Autoplay", &player->autoplay);
	float speed = player->speed;
	if (ImGui::InputFloat("Speed", &speed, 0, 0, "%.2f", ImGuiInputTextFlags_EnterReturnsTrue) &&
		std::isfinite(speed) && speed >= 0) {
		player->speed = speed;
		changed = true;
	}
	if (selected) {
		_animationPreview.Stop();
		_animationTimeline.Reset();
	}
	ImGui::EndDisabled();
	ImGui::BeginDisabled(!editor::CanEdit(Selected(), playing));
	const auto *current = library->FindClip(player->currentAnimation);
	if (ImGui::Checkbox("Playing", &player->isPlaying) && player->isPlaying && current &&
		_animationPreview.Time() >= current->duration) {
		_animationPreview.Seek(0);
		player->isPlaying = true;
	}
	if (ImGui::Button("Edit Animation", {-1, 0})) {
		_showAnimationEditor = true;
		ImGui::SetWindowFocus("Animation");
	}
	ImGui::EndDisabled();
	if (changed) {
		const auto data = saveAnimation(*library);
		auto after = *player;
		after.isPlaying = false;
		editor::RecordAnimationEdit(_sceneSaveStates, _editingScene, Selected(), resources,
			library->Id(), before, after, data, data);
	}
	ImGui::PopID();
}

void Editor::drawAnimationEditor(World &world, ResourceRegistry &resources, bool playing) {
	if (multiSelected()) {
		return;
	}
	if (!_showAnimationEditor) {
		return;
	}
	ImGui::SetNextWindowSize({980, 420}, ImGuiCond_FirstUseEver);
	if (!ImGui::Begin("Animation", &_showAnimationEditor)) {
		ImGui::End();
		return;
	}
	auto *player = Selected().Get<AnimationPlayer>();
	auto library = player ? resources.Get<AnimationLibrary>(player->library) : nullptr;
	if (!player || !library) {
		ImGui::TextDisabled("Select an entity with an AnimationPlayer.");
		ImGui::End();
		return;
	}
	ImGui::BeginDisabled(!editor::CanEdit(Selected(), playing));
	const auto beforePlayer = *player;
	auto beforeLibrary = *library;
	bool changed = false;
	ResourceID createdKeyResource;
	ImGui::TextUnformatted(Selected().GetName().c_str());
	ImGui::SameLine();
	ImGui::SetNextItemWidth(200);
	if (animationPicker(*player, *library, "##animation")) {
		_animationPreview.Stop();
		_animationTimeline.Reset();
		changed = true;
	}
	ImGui::SameLine();
	if (ImGui::Button("New")) {
		AnimationClip clip;
		int suffix = 1;
		do {
			clip.name = "Animation " + std::to_string(suffix++);
		} while (library->FindClip(clip.name));
		library->clips.push_back(clip);
		player->currentAnimation = clip.name;
		_animationPreview.Stop();
		_animationTimeline.Reset();
		changed = true;
	}
	AnimationClip *clip = nullptr;
	for (auto &item : library->clips) {
		if (item.name == player->currentAnimation) {
			clip = &item;
		}
	}
	if (clip) {
		ImGui::SameLine();
		if (ImGui::Button("Rename")) {
			ImGui::OpenPopup("rename_animation");
		}
		if (ImGui::BeginPopup("rename_animation")) {
			static std::string name;
			if (ImGui::IsWindowAppearing()) {
				name = clip->name;
				ImGui::SetKeyboardFocusHere();
			}
			const bool enter =
				ImGui::InputText("Name", &name, ImGuiInputTextFlags_EnterReturnsTrue);
			if (enter || ImGui::Button("Rename")) {
				if (!name.empty() && (clip->name == name || !library->FindClip(name))) {
					clip->name = name;
					player->currentAnimation = name;
					changed = true;
					ImGui::CloseCurrentPopup();
				}
			}
			ImGui::EndPopup();
		}
		ImGui::SameLine();
		if (ImGui::Button("Delete")) {
			const auto name = clip->name;
			std::erase_if(library->clips, [&](const auto &item) { return item.name == name; });
			player->currentAnimation = library->clips.empty() ? "" : library->clips.front().name;
			_animationPreview.Stop();
			_animationTimeline.Reset();
			clip = nullptr;
			changed = true;
		}
	}
	if (clip) {
		if (ImGui::Button(player->isPlaying ? "Pause" : "Play")) {
			const bool start = !player->isPlaying;
			if (start && _animationPreview.Time() >= clip->duration) {
				_animationPreview.Seek(0);
			}
			player->isPlaying = start;
		}
		ImGui::SameLine();
		if (ImGui::Button("Stop")) {
			_animationPreview.Stop();
		}
		ImGui::SameLine();
		ImGui::SetNextItemWidth(100);
		f64 cursor = _animationPreview.Time();
		if (ImGui::InputDouble("Time", &cursor, 0, 0, "%.3f",
				ImGuiInputTextFlags_EnterReturnsTrue)) {
			_animationPreview.Seek(std::clamp(cursor, 0., clip->duration));
		}
		ImGui::SameLine();
		ImGui::SetNextItemWidth(100);
		f64 duration = clip->duration;
		if (ImGui::InputDouble("Length", &duration, 0, 0, "%.3f",
				ImGuiInputTextFlags_EnterReturnsTrue) &&
			std::isfinite(duration) && duration >= .001) {
			clip->duration = duration;
			changed = true;
		}
		ImGui::SameLine();
		changed |= ImGui::Checkbox("Loop", &clip->loop);
		const f64 oldCursor = cursor;
		auto editResource = [&](ResourceID &value, const AnimationBinding &binding) {
			const auto component = world.FindComponent(binding.component);
			const auto *field =
				component.IsValid() ? component.FindField(binding.property) : nullptr;
			const auto selection = _resourceInspector.Draw("key_resource", value,
				field ? field->resourceType : "", _editingScene, resources, _fileSystem,
				_sceneSaveStates, editor::CanEdit(Selected(), playing));
			if (selection.changed) {
				value = selection.value;
				if (selection.created) {
					createdKeyResource = value;
				}
			}
			return selection.changed;
		};
		changed |= _animationTimeline.Draw(world, Selected(), *clip, cursor, editResource);
		player = Selected().Get<AnimationPlayer>();
		if (_animationTimeline.DidScrub() || cursor != oldCursor) {
			_animationPreview.Seek(cursor);
		}
	}
	if (changed) {
		// Playing is a preview control, not part of an authored edit or its undo.
		auto before = beforePlayer;
		auto after = *player;
		before.isPlaying = after.isPlaying = false;
		editor::RecordAnimationEdit(_sceneSaveStates, _editingScene, Selected(), resources,
			library->Id(), before, after, saveAnimation(beforeLibrary), saveAnimation(*library),
			false, createdKeyResource);
	}
	ImGui::EndDisabled();
	ImGui::End();
	if (!_showAnimationEditor) {
		_animationPreview.Stop();
	}
}

} // namespace sowa
