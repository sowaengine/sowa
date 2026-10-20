#include "editor/editor.hpp"

#include <imgui.h>

#include "editor/inspector.hpp"
#include "resource/sprite_sheet_animation.hpp"

namespace sowa {

void Editor::drawSpriteSheetEditor(ResourceRegistry &resources, bool playing) {
	if (multiSelected()) {
		return;
	}
	if (!_showSpriteSheetEditor) {
		return;
	}
	ImGui::SetNextWindowSize({1100, 540}, ImGuiCond_FirstUseEver);
	if (!ImGui::Begin("Sprite Sheet", &_showSpriteSheetEditor)) {
		ImGui::End();
		return;
	}

	auto *sprite = Selected().Get<AnimatedSprite>();
	if (!sprite) {
		ImGui::TextDisabled("Select an AnimatedSprite to edit its animations.");
		ImGui::End();
		return;
	}

	const bool editable = editor::CanEdit(Selected(), playing);
	ImGui::BeginDisabled(!editable);
	auto library = resources.Get<SpriteSheetAnimation>(sprite->animations);
	if (!library) {
		ImGui::TextWrapped(
			"Create an animation, choose its texture, then click the frames to play.");
		if (ImGui::Button("Create animation")) {
			const auto before = *sprite;
			const auto id =
				editor::CreateSceneResource(_editingScene, resources, "SpriteSheetAnimation");
			library = resources.Get<SpriteSheetAnimation>(id);
			if (library) {
				const auto empty = editor::SaveResource(*library);
				library->Set("Animation", {});
				sprite = Selected().Get<AnimatedSprite>();
				sprite->animations = id;
				sprite->currentAnimation = "Animation";
				editor::RecordAnimatedSpriteEdit(_sceneSaveStates, _editingScene, Selected(),
					resources, id, before, *sprite, empty, editor::SaveResource(*library), true);
			}
		}
	}

	if (library) {
		const auto before = *Selected().Get<AnimatedSprite>();
		const auto beforeData = editor::SaveResource(*library);
		auto animation = before.currentAnimation;
		ResourceID createdTexture;
		auto pickTexture = [&](ResourceID &texture) {
			const auto selection = _resourceInspector.Draw("texture", texture, "ImageTexture",
				_editingScene, resources, _fileSystem, _sceneSaveStates, editable);
			if (selection.changed) {
				texture = selection.value;
				if (selection.created) {
					createdTexture = texture;
				}
			}
			return selection.changed;
		};

		_spriteSheetPanel.Select(Selected(), library->Id());
		if (_spriteSheetPanel.Draw(*library, animation, resources, pickTexture)) {
			// Creating scene resources can move component storage.
			sprite = Selected().Get<AnimatedSprite>();
			sprite->currentAnimation = animation;
			sprite->frame = 0;
			sprite->playback = {};
			editor::RecordAnimatedSpriteEdit(_sceneSaveStates, _editingScene, Selected(), resources,
				library->Id(), before, *sprite, beforeData, editor::SaveResource(*library), false,
				createdTexture);
		}
	}

	ImGui::EndDisabled();
	ImGui::End();
}

} // namespace sowa
