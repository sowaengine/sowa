#include "animation/animated_sprite.hpp"

#include "ecs/world.hpp"
#include "resource/sprite_sheet_animation.hpp"
#include <algorithm>
#include <cmath>

namespace sowa {
void AdvanceAnimatedSprite(AnimatedSprite &sprite, ResourceRegistry &resources, f64 delta) {
	if (!std::isfinite(delta) || delta < 0) {
		return;
	}
	if (!sprite.playback.initialized) {
		sprite.isPlaying = sprite.isPlaying || sprite.autoplay;
	}
	if (!sprite.playback.initialized || sprite.playback.animations != sprite.animations ||
		sprite.playback.animation != sprite.currentAnimation) {
		sprite.playback = {sprite.animations, sprite.currentAnimation, 0, true};
		sprite.frame = 0;
	}
	const auto library = resources.Get<SpriteSheetAnimation>(sprite.animations);
	const auto *clip = library ? library->Find(sprite.currentAnimation) : nullptr;
	if (!clip || !clip->IsValid()) {
		sprite.frame = 0;
		sprite.isPlaying = false;
		return;
	}
	sprite.frame = std::clamp(sprite.frame, 0, clip->FrameCount() - 1);
	if (!sprite.isPlaying || !std::isfinite(sprite.speed) || sprite.speed <= 0 ||
		clip->Duration() <= 0) {
		return;
	}
	const f64 duration = double(clip->FrameCount()) / clip->framesPerSecond;
	const f64 end = sprite.playback.time + delta * sprite.speed;
	if (!std::isfinite(end)) {
		return;
	}
	sprite.playback.time = clip->loop ? std::fmod(end, duration) : std::min(end, duration);
	sprite.frame = static_cast<i32>(std::min(
		std::floor(sprite.playback.time * clip->framesPerSecond), double(clip->FrameCount() - 1)));
	if (!clip->loop && end >= duration) {
		sprite.isPlaying = false;
	}
}

bool PlayAnimatedSprite(Entity entity, ResourceRegistry &resources, const std::string &name) {
	auto *sprite = entity.Get<AnimatedSprite>();
	if (!sprite) {
		return false;
	}
	const auto library = resources.Get<SpriteSheetAnimation>(sprite->animations);
	const auto chosen = name.empty() ? sprite->currentAnimation : name;
	const auto *clip = library ? library->Find(chosen) : nullptr;
	if (!clip || !clip->IsValid()) {
		return false;
	}
	sprite->currentAnimation = chosen;
	sprite->playback = {sprite->animations, chosen, 0, true};
	sprite->frame = 0;
	sprite->isPlaying = true;
	return true;
}

void StopAnimatedSprite(Entity entity) {
	if (auto *sprite = entity.Get<AnimatedSprite>()) {
		sprite->isPlaying = false;
		sprite->frame = 0;
		sprite->playback = {sprite->animations, sprite->currentAnimation, 0, true};
	}
}

void RegisterAnimatedSpriteSystem(World &world, ResourceRegistry &resources) {
	if (world.Flecs().lookup("AnimateSprites")) {
		return;
	}
	auto builder = world.Flecs().system<AnimatedSprite>("AnimateSprites");
	auto system =
		builder.kind(flecs::OnValidate).each([&world, &resources](AnimatedSprite &sprite) {
			AdvanceAnimatedSprite(sprite, resources, world.DeltaTime());
		});
	world.TrackPlayOnly(System(system));
}
} // namespace sowa
