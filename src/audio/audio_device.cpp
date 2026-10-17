#include "audio/audio_device.hpp"

#include "audio/miniaudio/miniaudio_device.hpp"

namespace sowa {

Scope<AudioDevice> CreateAudioDevice(AudioAPI api) {
	switch (api) {
	case AudioAPI::MiniAudio:
		return MakeScope<MiniAudioDevice>();
	}
	return nullptr;
}

} // namespace sowa
