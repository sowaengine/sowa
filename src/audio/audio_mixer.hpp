#pragma once

#include <mutex>
#include <unordered_map>
#include <vector>

#include "audio/audio_source.hpp"
#include "core/types.hpp"
#include "ecs/entity.hpp"
#include "ecs/query.hpp"
#include "math/vector2.hpp"
#include "resource/resource_id.hpp"

namespace sowa {

class ResourceRegistry;
class World;

// Main thread: Sync after Progress; Play/Pause/Stop are safe during Progress.
// Audio thread: Mix only. Both use the same mutex; Mix reads sample snapshots, not ECS data.
class AudioMixer {
  public:
	// Registers AudioSource and builds the cached query. Call once per World.
	// World and resources must remain alive until Shutdown.
	void Init(World &world, ResourceRegistry &resources);
	// Stop the audio device before releasing mixer state.
	void Shutdown();

	// The listener is the active camera's world position. Rotation is used
	// only when Camera2D.rotatable is true (degrees, same as Transform2D).
	// Call before Sync. ClearListener when no camera is active.
	void SetListener(Vector2 position, f32 rotationDegrees = 0.f);
	void ClearListener();

	// Creates voices for new sources, drops dead ones, snapshots clip data.
	// Autoplay starts a voice the first time the entity is seen.
	void Sync();

	// Fills out with interleaved f32. Zeros the buffer first, then adds
	// every Playing voice. rate and channels are the destination stream.
	void Mix(f32 *out, u32 frames, u32 rate, u32 channels);

	// Stopped -> Playing from 0. Paused -> resume. Playing -> no-op.
	void Play(Entity entity);
	// Playing -> Paused. Otherwise no-op.
	void Pause(Entity entity);
	// Stopped, cursor 0. No-op if there is no voice.
	void Stop(Entity entity);

	bool IsPlaying(Entity entity) const;
	bool IsPaused(Entity entity) const;

	// Pauses mixing without changing voice states. Mix outputs silence and leaves cursors
	// unchanged; IsPlaying continues to report voices that will resume when unpaused.
	void SetPaused(bool paused);
	bool IsPaused() const;

  private:
	enum class VoiceState {
		Stopped,
		Playing,
		Paused,
	};

	struct Voice {
		VoiceState state = VoiceState::Stopped;
		f64 cursor = 0.0;
		ResourceID lastClip;
		Ref<const std::vector<f32>> sampleStorage;
		const f32 *samples = nullptr;
		u64 frameCount = 0;
		u32 clipRate = 0;
		u32 clipChannels = 0;
		f32 gainL = 1.f;
		f32 gainR = 1.f;
		f32 pitch = 1.f;
		bool loop = false;
	};

	void snapshotLocked(Entity entity, const AudioSource &src, Voice &voice);
	void applyGainsLocked(Entity entity, const AudioSource &src, Voice &voice);
	void mixVoice(Voice &voice, f32 *out, u32 frames, u32 rate, u32 channels);

	Query<AudioSource> _sources;
	ResourceRegistry *_resources = nullptr;
	mutable std::mutex _mutex;
	std::unordered_map<Entity, Voice> _voices;
	Vector2 _listenerPos;
	f32 _listenerRotation = 0.f;
	bool _listenerValid = false;
	bool _paused = false;
};

} // namespace sowa
