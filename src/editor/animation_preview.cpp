#include "editor/animation_preview.hpp"

#include <algorithm>
#include <cmath>

#include "ecs/components/camera2d.hpp"
#include "ecs/world.hpp"
#include "resource/resource_registry.hpp"

namespace sowa::editor {

void AnimationPreview::Select(Entity entity) {
	if (_entity == entity) {
		return;
	}
	Stop();
	_entity = entity;
	_animation.clear();
	_library = {};
}

void AnimationPreview::Stop() {
	if (auto *player = _entity.Get<AnimationPlayer>()) {
		player->isPlaying = false;
	}
	_active = false;
	_time = 0;
	_cameraViews.clear();
}

void AnimationPreview::Seek(f64 time) {
	if (!std::isfinite(time) || time < 0) {
		return;
	}
	_time = time;
	_active = true;
	if (auto *player = _entity.Get<AnimationPlayer>()) {
		player->isPlaying = false;
		_animation = player->currentAnimation;
		_library = player->library;
	}
}

void AnimationPreview::Apply(World &world, ResourceRegistry &resources, f64 delta,
	Vector2 viewportSize) {
	Restore(world);
	_cameraViews.clear();
	auto *player = _entity.Get<AnimationPlayer>();
	if (!player || !_entity.IsEnabled()) {
		return;
	}
	if (player->currentAnimation != _animation || player->library != _library) {
		_time = 0;
		_active = false;
		_animation = player->currentAnimation;
		_library = player->library;
	}
	auto library = resources.Get<AnimationLibrary>(player->library);
	const auto *clip = library ? library->FindClip(player->currentAnimation) : nullptr;
	if (!clip || !std::isfinite(clip->duration) || clip->duration <= 0) {
		Stop();
		return;
	}
	if (player->isPlaying) {
		_active = true;
		if (std::isfinite(delta) && delta >= 0 && std::isfinite(player->speed) &&
			player->speed > 0) {
			_time += delta * player->speed;
		}
		if (!std::isfinite(_time)) {
			_time = 0;
		}
		if (_time >= clip->duration) {
			_time = clip->loop ? std::fmod(_time, clip->duration) : clip->duration;
			player->isPlaying = clip->loop;
		}
	}
	if (!_active) {
		return;
	}
	_time = std::clamp(_time, 0., clip->duration);

	for (const auto &track : clip->tracks) {
		if (auto value = ReadAnimationProperty(world, _entity, track.binding)) {
			_originals.emplace_back(track.binding, *value);
		}
	}
	SampleAnimation(world, _entity, *clip, _time);
	_viewportSize = viewportSize;
	// Include cameras whose parent transforms or camera settings were animated.
	world.Each<Camera2D>([&](Entity camera, Camera2D &) {
		_cameraViews.emplace_back(camera, CalculateViewProjection(camera, viewportSize));
	});
}

glm::mat4 AnimationPreview::CameraViewProjection(Entity camera, Vector2 viewportSize) const {
	if (camera.IsAlive() && viewportSize == _viewportSize) {
		for (const auto &[entity, view] : _cameraViews) {
			if (entity == camera) {
				return view;
			}
		}
	}
	return CalculateViewProjection(camera, viewportSize);
}

void AnimationPreview::Restore(World &world) {
	for (const auto &[binding, value] : _originals) {
		WriteAnimationProperty(world, _entity, binding, value);
	}
	_originals.clear();
}

} // namespace sowa::editor
