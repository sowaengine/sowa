#pragma once

#include "core/types.hpp"

namespace sowa {

enum class AudioAPI {
	MiniAudio,
};

// Audio output stream and callback. AudioClip owns decoded PCM; AudioMixer mixes
// voices in the callback. Created through the backend factory, like GraphicsDevice.
struct AudioDeviceConfig {
	// Open a silent null device. Tests set this so they do not need hardware.
	bool nullDevice = false;
};

class AudioMixer;

class AudioDevice {
  public:
	virtual ~AudioDevice() = default;

	virtual AudioAPI API() const = 0;

	virtual bool Init(const AudioDeviceConfig &config) = 0;
	virtual void Shutdown() = 0;
	virtual bool IsInitialized() const = 0;

	// Native rate and channel count of the opened playback stream. Zero
	// until Init succeeds.
	virtual u32 SampleRate() const = 0;
	virtual u32 ChannelCount() const = 0;

	// The callback mixes this mixer, or silence if null. Set before Init.
	// Stop the device before changing or destroying the mixer.
	virtual void SetMixer(AudioMixer *mixer) = 0;
};

Scope<AudioDevice> CreateAudioDevice(AudioAPI api);

} // namespace sowa
