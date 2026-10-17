#pragma once

#include <string>
#include <vector>

#include "core/types.hpp"
#include "resource/resource.hpp"
#include "resource/resource_context.hpp"

namespace sowa {

// Fully decoded, backend-independent audio samples. Reload needs only a filesystem;
// AudioMixer retains sample snapshots for playback without an audio device on the resource.
class AudioClip : public Resource {
  public:
	// Path in the virtual filesystem. WAV (any bit depth dr_wav handles) and
	// Ogg Vorbis; anything else fails to load and says so.
	std::string path;

	void Reload(const ResourceContext &context) override;
	void Unload(const ResourceContext &context) override;

	// Interleaved f32 samples: stereo frame i occupies samples[i * 2] and samples[i * 2 + 1].
	// Matches the mixer output format.
	const std::vector<f32> &Samples() const {
		return *_samples;
	}

	Ref<const std::vector<f32>> SampleStorage() const {
		return _samples;
	}

	i32 SampleRate() const {
		return _sampleRate;
	}

	i32 ChannelCount() const {
		return _channels;
	}

	// One frame is one sample *per channel*, so this is Samples().size() /
	// ChannelCount() - the unit durations and playback positions are in.
	u64 FrameCount() const {
		return _frameCount;
	}

	// Seconds. Zero for a clip that never loaded.
	f32 Duration() const;

	bool IsLoaded() const {
		return !_samples->empty();
	}

  private:
	Ref<std::vector<f32>> _samples = MakeRef<std::vector<f32>>();
	i32 _sampleRate = 0;
	i32 _channels = 0;
	u64 _frameCount = 0;
};

} // namespace sowa
