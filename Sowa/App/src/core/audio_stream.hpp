#ifndef SW_AUDIO_STREAM_HPP_
#define SW_AUDIO_STREAM_HPP_

#include <cstdint>

namespace sowa {
class AudioStream {
  public:
	AudioStream();
	~AudioStream();

	void Play();

	void Load(unsigned char *data, size_t length);
	void Delete();

  private:
	unsigned int m_bufferId = 0;
	unsigned int m_sourceId = 0;

	float pitch = 1.f;
	float gain = 1.f;
	float position[3] = {0, 0, 0};
	float velocity[3] = {0, 0, 0};
	bool loop = false;
};
} // namespace sowa

#endif // SW_AUDIO_STREAM_HPP_