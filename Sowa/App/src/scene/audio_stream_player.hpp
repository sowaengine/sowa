#ifndef SW_AUDIO_STREAM_PLAYER_HPP_
#define SW_AUDIO_STREAM_PLAYER_HPP_

#include "core/audio_stream.hpp"
#include "node.hpp"
#include "sowa.hpp"

namespace sowa {
class AudioStreamPlayer : public Node {
  public:
	static std::string Typename() { return "sowa::AudioStreamPlayer"; }
	static void Bind();

	void UpdateLogic() override;

	inline Reference<AudioStream> &Stream() { return m_Stream; }
	inline bool &AutoStart() { return m_autoStart; }
	inline float &Pitch() { return m_Pitch; }
	inline float &Gain() { return m_Gain; }
	inline bool &Loop() { return m_Loop; }

	void Play();
	void Pause();
	void Stop();

	bool IsPlaying();

	AudioStreamPlayer();
	~AudioStreamPlayer();

  private:
	Reference<AudioStream> m_Stream = nullptr;

	uint32_t m_SourceId = 0;
	uint32_t m_LastBuffer = 0;

	bool m_autoStart = false;
	float m_Pitch = 1.f;
	float m_Gain = 1.f;
	bool m_Loop = false;
};
} // namespace sowa

#endif // SW_AUDIO_STREAM_PLAYER_HPP_