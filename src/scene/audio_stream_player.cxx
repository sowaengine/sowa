#include "audio_stream_player.hxx"

#include "resource/audio_stream/audio_stream.hxx"
#include "resource/resource_manager.hxx"
#include "servers/audio_server.hxx"
#include "utils/utils.hxx"

#include "AL/al.h"

AudioStreamPlayer::AudioStreamPlayer() {
	alGenSources(1, &m_source_id);
	alSourcef(m_source_id, AL_PITCH, 1.f);
	alSourcef(m_source_id, AL_GAIN, 1.f);
	alSourcei(m_source_id, AL_LOOPING, false);
}

AudioStreamPlayer::~AudioStreamPlayer() {
	stop();
	alDeleteSources(1, &m_source_id);
}

void AudioStreamPlayer::_exit() {
	stop();
}

void AudioStreamPlayer::play() {
	AudioStream *stream = ResourceManager::get().GetAs<AudioStream>(m_stream);
	if (nullptr == stream)
		return;

	if (stream->ID() == 0)
		return;

	if (m_last_buffer != stream->ID()) {
		m_last_buffer = stream->ID();
		alSourcei(m_source_id, AL_BUFFER, m_last_buffer);
	}

	alSourcePlay(m_source_id);
}

void AudioStreamPlayer::pause() {
	AudioStream *stream = ResourceManager::get().GetAs<AudioStream>(m_stream);
	if (nullptr == stream)
		return;

	if (stream->ID() == 0)
		return;

	alSourcePause(m_source_id);
}

void AudioStreamPlayer::stop() {
	AudioStream *stream = ResourceManager::get().GetAs<AudioStream>(m_stream);
	if (nullptr == stream)
		return;

	if (stream->ID() == 0)
		return;

	alSourceStop(m_source_id);
}

bool AudioStreamPlayer::is_playing() {
	int playing = 0;
	alGetSourcei(m_source_id, AL_SOURCE_STATE, &playing);

	return playing == AL_PLAYING;
}

void AudioStreamPlayer::_start() {
	if (m_autoplay)
		play();
}