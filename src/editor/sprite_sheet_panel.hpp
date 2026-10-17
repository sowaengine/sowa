#pragma once

#include <functional>
#include <string>

#include "ecs/entity.hpp"
#include "resource/sprite_sheet_animation.hpp"

namespace sowa {
class ImageTexture;
class ResourceRegistry;
} // namespace sowa

namespace sowa::editor {

bool DrawSpriteAnimationPicker(const char *label, const SpriteSheetAnimation *library,
	std::string &animation);

// UI state and drawing only. Editor owns the scene and records undoable edits.
class SpriteSheetPanel {
  public:
	using TexturePicker = std::function<bool(ResourceID &)>;

	void Select(Entity entity, ResourceID library);
	bool Draw(SpriteSheetAnimation &library, std::string &animation, ResourceRegistry &resources,
		const TexturePicker &pickTexture);

  private:
	bool drawToolbar(SpriteSheetAnimation &library, std::string &animation);
	bool drawSettings(SpriteAnimation &clip, const TexturePicker &pickTexture);
	bool drawSheet(SpriteAnimation &clip, const ImageTexture *texture);
	bool drawFrames(SpriteAnimation &clip, const ImageTexture *texture);
	void drawPreview(const SpriteAnimation &clip, const ImageTexture *texture);
	void resetPreview();

	Entity _entity;
	ResourceID _library;
	std::string _animation;
	float _zoom = 1.f;
	int _selectedFrame = 0;
	double _time = 0;
	bool _playing = false;
	bool _fitSheet = true;
	u64 _textureId = 0;
	std::string _name;
};

} // namespace sowa::editor
