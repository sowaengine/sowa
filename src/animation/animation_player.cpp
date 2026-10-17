#include "animation/animation.hpp"

#include <algorithm>
#include <cmath>

#include "core/log.hpp"
#include "ecs/world.hpp"
#include "resource/resource_registry.hpp"

namespace sowa {
namespace {
bool valid(const AnimationClip *clip) {
	return clip && std::isfinite(clip->duration) && clip->duration > 0;
}

f64 playbackTime(const AnimationClip &clip, f64 time) {
	return clip.loop ? std::fmod(time, clip.duration) : std::min(time, clip.duration);
}

void resetPlayback(AnimationPlayer &player) {
	player.playback = {player.library, player.currentAnimation, 0, true, true};
}

std::vector<AnimationCall> crossedCalls(const AnimationClip &clip, f64 start, f64 end,
	bool firstFrame) {
	struct TimedCall {
		f64 time;
		AnimationCall call;
	};

	std::vector<TimedCall> pending;
	constexpr size_t maxCalls = 4096;

	for (const AnimationCall &call : clip.calls) {
		if (!std::isfinite(call.time) || call.time < 0 || call.time > clip.duration ||
			call.method.empty()) {
			continue;
		}
		for (f64 time = call.time; time <= end;) {
			if (time > start || (firstFrame && time == start)) {
				pending.push_back({time, call});
			}
			if (!clip.loop || pending.size() >= maxCalls) {
				break;
			}
			const f64 next = time + clip.duration;
			if (next <= time) {
				break;
			}
			time = next;
		}
		if (pending.size() >= maxCalls) {
			Log::Warn("Animation event limit reached (4096 per update)");
			break;
		}
	}
	std::stable_sort(pending.begin(), pending.end(), [](const auto &a, const auto &b) {
		if (a.time != b.time) {
			return a.time < b.time;
		}
		return a.call.time > b.call.time; // end keys precede the next loop's start
	});
	std::vector<AnimationCall> calls;
	for (auto &entry : pending) {
		calls.push_back(std::move(entry.call));
	}
	return calls;
}
} // namespace

void SampleAnimation(World &world, Entity entity, const AnimationClip &clip, f64 time) {
	for (const AnimationTrack &track : clip.tracks) {
		if (auto value = track.Sample(time)) {
			WriteAnimationProperty(world, entity, track.binding, *value);
		}
	}
}

bool PlayAnimation(World &world, ResourceRegistry &resources, Entity entity,
	const std::string &name) {
	auto *player = entity.Get<AnimationPlayer>();
	if (!player) {
		return false;
	}
	auto library = resources.Get<AnimationLibrary>(player->library);
	if (!library) {
		return false;
	}
	const std::string selected = name.empty() ? player->currentAnimation : name;
	const auto *clip = library->FindClip(selected);
	if (!valid(clip)) {
		return false;
	}

	player->currentAnimation = selected;
	player->isPlaying = true;
	resetPlayback(*player);
	SampleAnimation(world, entity, *clip, 0);
	return true;
}

void PauseAnimation(Entity entity, bool paused) {
	if (auto *player = entity.Get<AnimationPlayer>()) {
		player->isPlaying = !paused;
	}
}

void StopAnimation(Entity entity) {
	if (auto *player = entity.Get<AnimationPlayer>()) {
		player->isPlaying = false;
		resetPlayback(*player);
	}
}

bool SeekAnimation(World &world, ResourceRegistry &resources, Entity entity, f64 time) {
	if (!std::isfinite(time) || time < 0) {
		return false;
	}
	auto *player = entity.Get<AnimationPlayer>();
	if (!player) {
		return false;
	}
	auto library = resources.Get<AnimationLibrary>(player->library);
	if (!library) {
		return false;
	}
	const auto *clip = library->FindClip(player->currentAnimation);
	if (!valid(clip)) {
		return false;
	}

	resetPlayback(*player);
	player->playback.time = playbackTime(*clip, time);
	player->playback.firstFrame = false;
	SampleAnimation(world, entity, *clip, player->playback.time);
	return true;
}

void AdvanceAnimation(World &world, ResourceRegistry &resources, Entity entity, f64 delta,
	const AnimationCallHandler &handler) {
	if (!std::isfinite(delta) || delta < 0) {
		return;
	}
	auto *player = entity.Get<AnimationPlayer>();
	if (!player) {
		return;
	}
	if (!player->playback.initialized) {
		player->isPlaying = player->isPlaying || player->autoplay;
		resetPlayback(*player);
	}
	if (player->playback.clip != player->currentAnimation ||
		player->playback.library != player->library) {
		resetPlayback(*player);
	}
	if (!player->isPlaying || !std::isfinite(player->speed) || player->speed <= 0) {
		return;
	}

	auto library = resources.Get<AnimationLibrary>(player->library);
	const auto *clip = library ? library->FindClip(player->currentAnimation) : nullptr;
	if (!valid(clip)) {
		player->isPlaying = false;
		return;
	}
	const f64 end = player->playback.time + delta * player->speed;
	if (!std::isfinite(end)) {
		return;
	}
	auto calls = crossedCalls(*clip, player->playback.time, end, player->playback.firstFrame);
	const f64 time = playbackTime(*clip, end);
	player->playback.time = time;
	player->playback.firstFrame = false;
	player->isPlaying = clip->loop || end < clip->duration;

	// Visibility can move component storage. Commit playback before applying the pose.
	SampleAnimation(world, entity, *clip, time);
	for (const auto &call : calls) {
		Entity target = ResolveAnimationTarget(entity, call.path);
		if (handler && target.IsAlive()) {
			handler(target, call.component, call.method);
		}
	}
}

void RegisterAnimationSystem(World &world, ResourceRegistry &resources,
	AnimationCallHandler handler) {
	auto system =
		world.Flecs()
			.system("AnimateProperties")
			.kind(flecs::OnValidate)
			.run([&world, &resources, handler](flecs::iter &) {
				std::vector<Entity> players;
				world.Each<AnimationPlayer>(
					[&](Entity entity, AnimationPlayer &) { players.push_back(entity); });
				for (Entity entity : players) {
					AdvanceAnimation(world, resources, entity, world.DeltaTime(), handler);
				}
			});
	world.TrackPlayOnly(System(system));
}
} // namespace sowa
