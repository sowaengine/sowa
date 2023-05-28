#include "audio_stream_player.hpp"

#include "AL/al.h"

#include "core/node_db.hpp"
#include "resource/resource_loader.hpp"

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

void AudioStreamPlayer::Bind() {
	NodeFactory factory;
	factory.construct = []() -> Node * {
		Node *node = Allocator<AudioStreamPlayer>::Get().allocate(1);
		new (node) AudioStreamPlayer;

		return node;
	};

	factory.destruct = [](Node *node) {
		Allocator<AudioStreamPlayer>::Get().deallocate(reinterpret_cast<AudioStreamPlayer *>(node), 1);
	};

	NodeDB::Instance().RegisterNodeType("AudioStreamPlayer", "Node", factory);

	NodeDB::Instance().RegisterAttribute(
		"AudioStreamPlayer", "stream.path", [](Node *node) -> light_variant {
		Debug::Error("Impelement AudioStreamPlayer Attribute getter texture.path");
		return ""; }, [](Node *node, light_variant path) {
		AudioStreamPlayer* player = dynamic_cast<AudioStreamPlayer*>(node);
		if(nullptr != player) {
			player->Stream() = ResourceLoader::get_singleton().LoadResource<AudioStream>(path.String());
		} });

	NodeDB::Instance().RegisterAttribute(
		"AudioStreamPlayer", "autostart", [](Node *node) -> light_variant {
		AudioStreamPlayer* player = dynamic_cast<AudioStreamPlayer*>(node);
		if(nullptr != player) {
			return player->AutoStart();
		}
		return false; }, [](Node *node, light_variant start) {
		AudioStreamPlayer* player = dynamic_cast<AudioStreamPlayer*>(node);
		if(nullptr != player) {
			player->AutoStart() = start.Bool();
		} });

	NodeDB::Instance().RegisterAttribute(
		"AudioStreamPlayer", "playing", [](Node *node) -> light_variant {
		AudioStreamPlayer* player = dynamic_cast<AudioStreamPlayer*>(node);
		if(nullptr != player) {
			return player->IsPlaying();
		}
		return false; }, [](Node *node, light_variant play) {
		AudioStreamPlayer* player = dynamic_cast<AudioStreamPlayer*>(node);
		if(nullptr != player) {
			if(play.Bool()) {
				player->Play();
			}
			else {
				player->Stop();
			}
		} });
}

void AudioStreamPlayer::UpdateLogic() {
	if (m_autoStart && m_Stream != nullptr && m_Stream->ID() != 0) {
		m_autoStart = false;
		Play();
	}
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

	return playing == AL_PLAYING;
}
} // namespace sowa
