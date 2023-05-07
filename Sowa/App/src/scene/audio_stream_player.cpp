#include "audio_stream_player.hpp"

#include "AL/al.h"

namespace sowa {
AudioStreamPlayer::AudioStreamPlayer() {
    m_type = Typename();
    _NodeType = "AudioStreamPlayer";

	alGenSources(1, &m_SourceId);
	alSourcef(m_SourceId, AL_PITCH, m_Pitch);
	alSourcef(m_SourceId, AL_GAIN, m_Gain);
	alSourcei(m_SourceId, AL_LOOPING, m_Loop);
}
AudioStreamPlayer::~AudioStreamPlayer() {
	alDeleteSources(1, &m_SourceId);
}
void AudioStreamPlayer::Play() {
	if (m_Stream == nullptr) {
		return;
	}
	if (m_Stream->ID() == 0) {
		return;
	}

	if (m_LastBuffer != m_Stream->ID()) {
		m_LastBuffer = m_Stream->ID();

		alSourcei(m_SourceId, AL_BUFFER, m_LastBuffer);
	}

	// Todo: update values on change
	alSourcef(m_SourceId, AL_PITCH, m_Pitch);
	alSourcef(m_SourceId, AL_GAIN, m_Gain);
	alSourcei(m_SourceId, AL_LOOPING, m_Loop);

	alSourcePlay(m_LastBuffer);
}

void AudioStreamPlayer::Pause() {
	if (m_LastBuffer == 0) {
		return;
	}

	alSourcePause(m_SourceId);
}
void AudioStreamPlayer::Stop() {
	if (m_LastBuffer == 0) {
		return;
	}

	alSourceStop(m_SourceId);
}

bool AudioStreamPlayer::IsPlaying() {
    int playing = 0;
    alGetSourcei(m_SourceId, AL_SOURCE_STATE, &playing);

    return playing == AL_PLAYING;;
}
} // namespace sowa
