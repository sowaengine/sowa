#include "ECS/Components/AudioStreamPlayer/AudioStreamPlayer.hpp"
#include "Resource/ResourceManager.hpp"

namespace Ease::Component {
AudioStreamPlayer::AudioStreamPlayer() {
}

AudioStreamPlayer::~AudioStreamPlayer() {
}

void AudioStreamPlayer::Play() {
	m_Source.Play(GetStream());
}
void AudioStreamPlayer::Stop() {
	m_Source.Stop(GetStream());
}

std::shared_ptr<Ease::AudioStream> &AudioStreamPlayer::GetStream() {
	// return Ease::ResourceManager<Ease::AudioStream>::GetLoader().GetResource(m_Stream);
	assert(false);
	static std::shared_ptr<Ease::AudioStream> nullret{nullptr};
	return nullret;
}
} // namespace Ease::Component