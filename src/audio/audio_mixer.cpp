#include "audio/audio_mixer.hpp"

#include <cmath>
#include <cstring>
#include <unordered_set>

#include "ecs/components/transform2d.hpp"
#include "ecs/world.hpp"
#include "math/math.hpp"
#include "resource/audio_clip.hpp"
#include "resource/resource_registry.hpp"

namespace sowa {

void AudioMixer::Init(World &world, ResourceRegistry &resources) {
	_resources = &resources;

	world.RegisterComponent<AudioSource>("AudioSource")
		.ResourceField("clip", &AudioSource::clip, "AudioClip")
		.Field("gain", &AudioSource::gain)
		.Field("pitch", &AudioSource::pitch)
		.Field("loop", &AudioSource::loop)
		.Field("autoplay", &AudioSource::autoplay)
		.Field("pan", &AudioSource::pan)
		.Field("spatial", &AudioSource::spatial)
		.Field("maxDistance", &AudioSource::maxDistance);

	_sources = world.CreateQuery<AudioSource>();
}

void AudioMixer::Shutdown() {
	std::lock_guard lock(_mutex);
	_voices.clear();
	_sources = {};
	_resources = nullptr;
	_listenerPos = {};
	_listenerRotation = 0.f;
	_listenerValid = false;
	_paused = false;
}

void AudioMixer::SetListener(Vector2 position, f32 rotationDegrees) {
	std::lock_guard lock(_mutex);
	_listenerPos = position;
	_listenerRotation = rotationDegrees;
	_listenerValid = true;
}

void AudioMixer::ClearListener() {
	std::lock_guard lock(_mutex);
	_listenerValid = false;
}

void AudioMixer::applyGainsLocked(Entity entity, const AudioSource &src, Voice &voice) {
	f32 pan = src.pan;
	f32 atten = 1.f;

	if (src.spatial) {
		const Transform2D *transform = entity.Get<Transform2D>();
		if (transform != nullptr && _listenerValid) {
			Vector2 delta = transform->WorldPosition() - _listenerPos;
			if (_listenerRotation != 0.f) {
				const f32 rad = -_listenerRotation * Math::Pi / 180.f;
				const f32 c = Math::Cos(rad);
				const f32 s = Math::Sin(rad);
				delta = Vector2(delta.x * c - delta.y * s, delta.x * s + delta.y * c);
			}
			if (src.maxDistance <= 0.f) {
				atten = 0.f;
				pan = 0.f;
			} else {
				const f32 distance = delta.Length();
				atten = 1.f - Math::Clamp(distance / src.maxDistance, 0.f, 1.f);
				pan = Math::Clamp(delta.x / src.maxDistance, -1.f, 1.f);
			}
		} else {
			pan = 0.f;
			atten = 1.f;
		}
	}

	pan = Math::Clamp(pan, -1.f, 1.f);
	const f32 angle = (pan + 1.f) * (Math::Pi * 0.25f);
	voice.gainL = src.gain * atten * Math::Cos(angle);
	voice.gainR = src.gain * atten * Math::Sin(angle);
}

void AudioMixer::snapshotLocked(Entity entity, const AudioSource &src, Voice &voice) {
	if (src.clip != voice.lastClip) {
		voice.lastClip = src.clip;
		voice.cursor = 0.0;
	}

	voice.pitch = src.pitch;
	voice.loop = src.loop;
	applyGainsLocked(entity, src, voice);

	voice.sampleStorage.reset();
	voice.samples = nullptr;
	voice.frameCount = 0;
	voice.clipRate = 0;
	voice.clipChannels = 0;
	if (_resources == nullptr || !src.clip.IsValid()) {
		return;
	}

	const Ref<AudioClip> clip = _resources->Get<AudioClip>(src.clip);
	if (clip == nullptr || !clip->IsLoaded()) {
		return;
	}

	voice.sampleStorage = clip->SampleStorage();
	voice.samples = voice.sampleStorage->data();
	voice.frameCount = clip->FrameCount();
	voice.clipRate = static_cast<u32>(clip->SampleRate());
	voice.clipChannels = static_cast<u32>(clip->ChannelCount());
}

void AudioMixer::Sync() {
	if (!_sources.IsValid() || _resources == nullptr) {
		return;
	}

	std::unordered_set<Entity> live;
	std::lock_guard lock(_mutex);
	_sources.Each([&](Entity entity, AudioSource &src) {
		live.insert(entity);
		const auto it = _voices.find(entity);
		const bool created = it == _voices.end();
		Voice &voice = created ? _voices[entity] : it->second;
		if (created) {
			voice.state = src.autoplay ? VoiceState::Playing : VoiceState::Stopped;
			voice.cursor = 0.0;
			voice.lastClip = {};
		}
		snapshotLocked(entity, src, voice);
	});

	for (auto it = _voices.begin(); it != _voices.end();) {
		if (!it->first.IsAlive() || live.find(it->first) == live.end()) {
			it = _voices.erase(it);
		} else {
			++it;
		}
	}
}

void AudioMixer::Play(Entity entity) {
	AudioSource *src = entity.Get<AudioSource>();
	if (src == nullptr) {
		return;
	}

	std::lock_guard lock(_mutex);
	Voice &voice = _voices[entity];
	if (voice.state == VoiceState::Playing) {
		return;
	}
	if (voice.state == VoiceState::Stopped) {
		voice.cursor = 0.0;
	}
	snapshotLocked(entity, *src, voice);
	voice.state = VoiceState::Playing;
}

void AudioMixer::Pause(Entity entity) {
	std::lock_guard lock(_mutex);
	const auto it = _voices.find(entity);
	if (it == _voices.end() || it->second.state != VoiceState::Playing) {
		return;
	}
	it->second.state = VoiceState::Paused;
}

void AudioMixer::Stop(Entity entity) {
	std::lock_guard lock(_mutex);
	const auto it = _voices.find(entity);
	if (it == _voices.end()) {
		return;
	}
	it->second.state = VoiceState::Stopped;
	it->second.cursor = 0.0;
}

bool AudioMixer::IsPlaying(Entity entity) const {
	std::lock_guard lock(_mutex);
	const auto it = _voices.find(entity);
	return it != _voices.end() && it->second.state == VoiceState::Playing;
}

bool AudioMixer::IsPaused(Entity entity) const {
	std::lock_guard lock(_mutex);
	const auto it = _voices.find(entity);
	return it != _voices.end() && it->second.state == VoiceState::Paused;
}

void AudioMixer::Mix(f32 *out, u32 frames, u32 rate, u32 channels) {
	if (out == nullptr || frames == 0 || channels == 0) {
		return;
	}

	std::memset(out, 0, static_cast<size_t>(frames) * channels * sizeof(f32));

	std::lock_guard lock(_mutex);
	if (_paused) {
		return;
	}
	for (auto &[entity, voice] : _voices) {
		(void)entity;
		mixVoice(voice, out, frames, rate, channels);
	}
}

void AudioMixer::SetPaused(bool paused) {
	std::lock_guard lock(_mutex);
	_paused = paused;
}

bool AudioMixer::IsPaused() const {
	std::lock_guard lock(_mutex);
	return _paused;
}

void AudioMixer::mixVoice(Voice &voice, f32 *out, u32 frames, u32 rate, u32 channels) {
	if (voice.state != VoiceState::Playing) {
		return;
	}
	if (voice.samples == nullptr || voice.frameCount == 0 || voice.clipRate == 0) {
		return;
	}
	if (voice.pitch <= 0.f || rate == 0) {
		return;
	}

	const f64 step =
		(static_cast<f64>(voice.clipRate) / static_cast<f64>(rate)) * static_cast<f64>(voice.pitch);
	const u32 srcCh = voice.clipChannels == 0 ? 1 : voice.clipChannels;

	auto sampleAt = [&](u64 frame, u32 channel) -> f32 {
		if (frame >= voice.frameCount) {
			return 0.f;
		}
		if (srcCh == 1) {
			return voice.samples[frame];
		}
		const u32 c = channel < srcCh ? channel : 0;
		return voice.samples[frame * srcCh + c];
	};

	auto channelGain = [&](u32 channel) -> f32 { return channel == 1 ? voice.gainR : voice.gainL; };

	for (u32 i = 0; i < frames; ++i) {
		if (voice.cursor >= static_cast<f64>(voice.frameCount)) {
			if (voice.loop) {
				voice.cursor = std::fmod(voice.cursor, static_cast<f64>(voice.frameCount));
			} else {
				voice.state = VoiceState::Stopped;
				voice.cursor = 0.0;
				break;
			}
		}

		const u64 i0 = static_cast<u64>(voice.cursor);
		const f32 frac = static_cast<f32>(voice.cursor - static_cast<f64>(i0));
		u64 i1 = i0 + 1;
		if (i1 >= voice.frameCount) {
			i1 = voice.loop ? 0 : i0;
		}

		for (u32 c = 0; c < channels; ++c) {
			const f32 a = sampleAt(i0, c);
			const f32 b = sampleAt(i1, c);
			out[i * channels + c] += (a + (b - a) * frac) * channelGain(c);
		}
		voice.cursor += step;
	}

	if (voice.state == VoiceState::Playing && voice.cursor >= static_cast<f64>(voice.frameCount)) {
		if (voice.loop) {
			voice.cursor = std::fmod(voice.cursor, static_cast<f64>(voice.frameCount));
		} else {
			voice.state = VoiceState::Stopped;
			voice.cursor = 0.0;
		}
	}
}

} // namespace sowa
