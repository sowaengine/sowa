#pragma once

#include "audio/audio_device.hpp"

namespace sowa {

struct MiniAudioPlayback;

class MiniAudioDevice : public AudioDevice {
  public:
	MiniAudioDevice();
	~MiniAudioDevice() override;

	MiniAudioDevice(const MiniAudioDevice &) = delete;
	MiniAudioDevice &operator=(const MiniAudioDevice &) = delete;

	AudioAPI API() const override {
		return AudioAPI::MiniAudio;
	}

	bool Init(const AudioDeviceConfig &config) override;
	void Shutdown() override;
	bool IsInitialized() const override;

	u32 SampleRate() const override;
	u32 ChannelCount() const override;

	void SetMixer(AudioMixer *mixer) override;

  private:
	friend struct MiniAudioPlayback;
	void writePlayback(void *output, u32 frameCount);

	struct Impl;
	Scope<Impl> _impl;
};

} // namespace sowa
