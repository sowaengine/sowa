#pragma once

#include <compare>
#include <functional>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

#include "ecs/entity.hpp"
#include "resource/resource.hpp"
#include "resource/resource_id.hpp"

namespace sowa {
class World;
class ResourceRegistry;

// Entity-reference values are relative paths (strings), never world-specific IDs.
using AnimationValue =
	std::variant<bool, i32, u32, f32, f64, std::string, Vector2, Vector3, Color, ResourceID>;
enum class KeyInterpolation {
	Step,
	Linear,
	Smooth,
	AngleDegrees
};

struct AnimationKey {
	u64 id = 0;
	f64 time = 0;
	AnimationValue value = 0.f;
	KeyInterpolation interpolation = KeyInterpolation::Linear;
};

struct AnimationBinding {
	std::string path = "."; // relative to the player; supports '..'
	std::string component;
	std::string property; // empty component + "isVisible" is entity visibility
	FieldType type = FieldType::F32;
	auto operator<=>(const AnimationBinding &) const = default;
};

class AnimationTrack {
  public:
	AnimationBinding binding;
	// Stable IDs survive time edits and sorting. Equal times replace the key.
	u64 Insert(f64 time, AnimationValue value,
		KeyInterpolation interpolation = KeyInterpolation::Linear);
	bool Update(u64 id, f64 time, AnimationValue value, KeyInterpolation interpolation);
	bool Remove(u64 id);

	const std::vector<AnimationKey> &Keys() const {
		return _keys;
	}

	std::optional<AnimationValue> Sample(f64 time) const;
	void Serialize(Archive &archive);

  private:
	std::vector<AnimationKey> _keys;
	u64 _nextId = 1;
};

struct AnimationCall {
	f64 time = 0;
	std::string path = ".";
	std::string component;
	std::string method; // no-argument component method
};

struct AnimationClip {
	std::string name;
	f64 duration = 1;
	bool loop = false;
	std::vector<AnimationTrack> tracks;
	std::vector<AnimationCall> calls;
};

class AnimationLibrary : public Resource {
  public:
	std::vector<AnimationClip> clips;
	const AnimationClip *FindClip(const std::string &name) const;
	void Serialize(Archive &archive) override;
};

struct AnimationPlayback {
	ResourceID library;
	std::string clip;
	f64 time = 0;
	bool initialized = false;
	bool firstFrame = true;
};

struct AnimationPlayer {
	ResourceID library; // scene-owned storage; managed by the editor
	std::string currentAnimation;
	bool autoplay = false;
	bool isPlaying = false;
	f32 speed = 1.f;
	AnimationPlayback playback; // runtime only
};

Entity ResolveAnimationTarget(Entity root, std::string_view path);
std::optional<AnimationValue> ReadAnimationProperty(World &world, Entity root,
	const AnimationBinding &binding);
bool WriteAnimationProperty(World &world, Entity root, const AnimationBinding &binding,
	const AnimationValue &value);
AnimationValue DefaultAnimationValue(FieldType type);
bool AnimationValueMatches(FieldType type, const AnimationValue &value);
AnimationValue InterpolateAnimationValues(const AnimationValue &a, const AnimationValue &b,
	f64 weight, KeyInterpolation interpolation = KeyInterpolation::Linear);

// Sample a pose without advancing time or invoking events.
void SampleAnimation(World &world, Entity player, const AnimationClip &clip, f64 time);
bool PlayAnimation(World &world, ResourceRegistry &resources, Entity player,
	const std::string &name = {});
void PauseAnimation(Entity player, bool paused = true);
void StopAnimation(Entity player);
bool SeekAnimation(World &world, ResourceRegistry &resources, Entity player, f64 time);

using AnimationCallHandler = std::function<void(Entity, const std::string &, const std::string &)>;
void AdvanceAnimation(World &world, ResourceRegistry &resources, Entity player, f64 delta,
	const AnimationCallHandler &handler = {});
// Call once before initializing physics. Gameplay runs before animation;
// physics and world-transform calculation consume the resulting pose.
void RegisterAnimationSystem(World &world, ResourceRegistry &resources,
	AnimationCallHandler handler = {});
} // namespace sowa
