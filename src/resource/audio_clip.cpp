#include "resource/audio_clip.hpp"

#include <cstring>

#include "core/filesystem/file_system.hpp"
#include "core/log.hpp"
#include "core/string.hpp"

#define DR_WAV_IMPLEMENTATION
#define DRWAV_NO_STDIO
#include <dr_wav.h>

// stb_vorbis.c is compiled as its own C translation unit (see CMakeLists), so
// only its declarations are wanted here.
#define STB_VORBIS_HEADER_ONLY
#include <stb_vorbis.c>

namespace sowa {

namespace {

// Decode directly from FileSystem bytes.

bool decodeWav(const FileData &file, std::vector<f32> &samples, i32 &sampleRate, i32 &channels,
	u64 &frameCount) {
	drwav wav;
	if (drwav_init_memory(&wav, file.Data(), file.Size(), nullptr) == DRWAV_FALSE) {
		return false;
	}

	samples.resize(static_cast<size_t>(wav.totalPCMFrameCount) * wav.channels);
	const drwav_uint64 read =
		drwav_read_pcm_frames_f32(&wav, wav.totalPCMFrameCount, samples.data());

	sampleRate = static_cast<i32>(wav.sampleRate);
	channels = static_cast<i32>(wav.channels);
	frameCount = read;
	// Short reads are truncated rather than left as trailing zeros, so
	// FrameCount always describes samples that actually exist.
	samples.resize(static_cast<size_t>(read) * wav.channels);

	drwav_uninit(&wav);
	return read > 0;
}

bool decodeVorbis(const FileData &file, std::vector<f32> &samples, i32 &sampleRate, i32 &channels,
	u64 &frameCount) {
	int error = 0;
	stb_vorbis *vorbis =
		stb_vorbis_open_memory(reinterpret_cast<const unsigned char *>(file.Data()),
			static_cast<int>(file.Size()), &error, nullptr);
	if (vorbis == nullptr) {
		return false;
	}

	const stb_vorbis_info info = stb_vorbis_get_info(vorbis);
	const unsigned int frames = stb_vorbis_stream_length_in_samples(vorbis);

	samples.resize(static_cast<size_t>(frames) * info.channels);
	// Decode directly to f32, matching the WAV path.
	const int read = stb_vorbis_get_samples_float_interleaved(vorbis, info.channels, samples.data(),
		static_cast<int>(samples.size()));

	sampleRate = static_cast<i32>(info.sample_rate);
	channels = info.channels;
	frameCount = static_cast<u64>(read);
	samples.resize(static_cast<size_t>(read) * info.channels);

	stb_vorbis_close(vorbis);
	return read > 0;
}

} // namespace

void AudioClip::Reload(const ResourceContext &context) {
	// Same contract as every other resource: make the object match its fields,
	// which starts with dropping whatever it holds.
	Unload(context);

	if (path.empty()) {
		return;
	}

	if (context.fs == nullptr) {
		Log::Error("AudioClip: can't load {} without a filesystem", path);
		return;
	}

	const auto file = context.fs->Load(path);
	if (!file) {
		Log::Error("AudioClip: failed to read {} (error {})", path, static_cast<int>(file.error));
		return;
	}
	if (!file.value || file.value->Size() == 0) {
		Log::Error("AudioClip: {} is empty", path);
		return;
	}

	// Dispatched on extension, then verified by the decoder actually accepting
	// the bytes - a mislabelled file fails loudly rather than decoding as
	// noise.
	const std::string lowered = String::ToLower(path);
	bool decoded = false;
	if (String::EndsWith(lowered, ".wav")) {
		decoded = decodeWav(*file.value, *_samples, _sampleRate, _channels, _frameCount);
	} else if (String::EndsWith(lowered, ".ogg")) {
		decoded = decodeVorbis(*file.value, *_samples, _sampleRate, _channels, _frameCount);
	} else {
		Log::Error("AudioClip: {} has no decoder - .wav and .ogg are supported", path);
		return;
	}

	// Reset all decoded state on failure, including metadata set by the decoder.
	if (!decoded || _channels <= 0 || _sampleRate <= 0) {
		Log::Error("AudioClip: failed to decode {}", path);
		Unload(context);
	}
}

void AudioClip::Unload(const ResourceContext &context) {
	// Nothing to hand back to a device - the samples are plain memory, which
	// is exactly the point of decoding to a backend-agnostic format.
	(void)context;

	_samples = MakeRef<std::vector<f32>>();
	_sampleRate = 0;
	_channels = 0;
	_frameCount = 0;
}

f32 AudioClip::Duration() const {
	if (_sampleRate <= 0) {
		return 0.f;
	}
	return static_cast<f32>(_frameCount) / static_cast<f32>(_sampleRate);
}

} // namespace sowa
