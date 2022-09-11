#ifndef _E_SOUNDSOURCE_HPP__
#define _E_SOUNDSOURCE_HPP__
#pragma once

#include "Resource/AudioStream/AudioStream.hpp"
#include <AL/al.h>
#include <glm/glm.hpp>
#include <memory>

namespace Ease::Component {
class AudioStreamPlayer;
}
namespace Ease {
class SoundSource {
  public:
	SoundSource();
	~SoundSource();

	void Play(std::shared_ptr<Ease::AudioStream> stream);
	void Stop(std::shared_ptr<Ease::AudioStream> stream);

	bool &Looping() { return m_Loop; }
	void UpdateData();

  private:
	friend class AudioStreamPlayer;

	ALuint m_Source;
	float m_Pitch = 1.f;
	float m_Gain = 1.f;
	glm::vec3 m_Pos;
	glm::vec3 m_Velocity;
	bool m_Loop = false;
	ALuint m_Buffer = 0;
};
} // namespace Ease

#endif // _E_SOUNDSOURCE_HPP__