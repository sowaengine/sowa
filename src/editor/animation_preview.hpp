#pragma once

#include <glm/glm.hpp>

#include "animation/animation.hpp"

namespace sowa::editor {

// The preview pose exists only during the render pass. Restore runs before
// inspector edits, saving, undo, or starting the game can see those values.
class AnimationPreview {
  public:
	void Select(Entity entity);
	void Stop();
	void Seek(f64 time);
	void Apply(World &world, ResourceRegistry &resources, f64 delta, Vector2 viewportSize);
	void Restore(World &world);
	// Camera outlines must use the rendered pose even after Restore.
	glm::mat4 CameraViewProjection(Entity camera, Vector2 viewportSize) const;

	f64 Time() const {
		return _time;
	}

	bool IsActive() const {
		return _active;
	}

  private:
	Entity _entity;
	ResourceID _library;
	std::string _animation;
	f64 _time = 0;
	bool _active = false;
	std::vector<std::pair<AnimationBinding, AnimationValue>> _originals;
	Vector2 _viewportSize;
	std::vector<std::pair<Entity, glm::mat4>> _cameraViews;
};

} // namespace sowa::editor
