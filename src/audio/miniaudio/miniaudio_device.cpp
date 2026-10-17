#include "audio/miniaudio/miniaudio_device.hpp"

#include <cstring>

#include "audio/audio_mixer.hpp"
#include "core/log.hpp"

// miniaudio 0.11.25 (thirdparty/miniaudio). Decode stays on AudioClip;
// this translation unit is the sink only.
#define MA_NO_DECODING
#define MA_NO_ENCODING
#define MA_NO_GENERATION
#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

namespace sowa {

struct MiniAudioPlayback {
	static void onPlayback(ma_device *device, void *output, const void *input,
		ma_uint32 frameCount) {
		(void)input;
		if (device == nullptr || device->pUserData == nullptr) {
			return;
		}
		static_cast<MiniAudioDevice *>(device->pUserData)->writePlayback(output, frameCount);
	}
};

struct MiniAudioDevice::Impl {
	ma_device device{};
	bool initialized = false;
	AudioMixer *mixer = nullptr;
};

MiniAudioDevice::MiniAudioDevice() : _impl(MakeScope<Impl>()) {
}

MiniAudioDevice::~MiniAudioDevice() {
	Shutdown();
}

bool MiniAudioDevice::Init(const AudioDeviceConfig &config) {
	Shutdown();

	ma_device_config deviceConfig = ma_device_config_init(ma_device_type_playback);
	deviceConfig.playback.format = ma_format_f32;
	deviceConfig.playback.channels = 2;
	deviceConfig.dataCallback = MiniAudioPlayback::onPlayback;
	deviceConfig.pUserData = this;

	ma_result result = MA_ERROR;
	if (config.nullDevice) {
		const ma_backend backends[] = {ma_backend_null};
		result = ma_device_init_ex(backends, 1, nullptr, &deviceConfig, &_impl->device);
	} else {
		result = ma_device_init(nullptr, &deviceConfig, &_impl->device);
	}

	if (result != MA_SUCCESS) {
		Log::Error("AudioDevice: miniaudio init failed ({})", ma_result_description(result));
		return false;
	}

	_impl->initialized = true;
	result = ma_device_start(&_impl->device);
	if (result != MA_SUCCESS) {
		Log::Error("AudioDevice: miniaudio start failed ({})", ma_result_description(result));
		ma_device_uninit(&_impl->device);
		_impl->device = {};
		_impl->initialized = false;
		return false;
	}

#if defined(__EMSCRIPTEN__)
	// Browsers keep the AudioContext suspended until a user gesture.
	// miniaudio resumes it on click/touchend only; key presses are gestures
	// too, and a keyboard-only player would otherwise never hear anything.
	EM_ASM({
		if (window.miniaudio && window.miniaudio.unlock) {
			document.addEventListener('keydown', window.miniaudio.unlock, true);
		}
	});
#endif

	return true;
}

void MiniAudioDevice::Shutdown() {
	if (_impl == nullptr || !_impl->initialized) {
		return;
	}
	ma_device_uninit(&_impl->device);
	_impl->device = {};
	_impl->initialized = false;
}

bool MiniAudioDevice::IsInitialized() const {
	return _impl != nullptr && _impl->initialized;
}

u32 MiniAudioDevice::SampleRate() const {
	if (!IsInitialized()) {
		return 0;
	}
	return _impl->device.sampleRate;
}

u32 MiniAudioDevice::ChannelCount() const {
	if (!IsInitialized()) {
		return 0;
	}
	return _impl->device.playback.channels;
}

void MiniAudioDevice::SetMixer(AudioMixer *mixer) {
	if (_impl != nullptr) {
		_impl->mixer = mixer;
	}
}

void MiniAudioDevice::writePlayback(void *output, u32 frameCount) {
	if (output == nullptr || _impl == nullptr || !_impl->initialized) {
		return;
	}

	const u32 channels = _impl->device.playback.channels;
	if (_impl->mixer == nullptr) {
		const size_t bytes = static_cast<size_t>(frameCount) * channels *
							 ma_get_bytes_per_sample(_impl->device.playback.format);
		std::memset(output, 0, bytes);
		return;
	}

	_impl->mixer->Mix(static_cast<f32 *>(output), frameCount, _impl->device.sampleRate, channels);
}

} // namespace sowa
