#ifndef SW_AUDIO_STREAM_HPP_
#define SW_AUDIO_STREAM_HPP_

#include <cstdint>

namespace sowa {
class AudioStream {
  public:
	AudioStream();
	~AudioStream();

	bool Load(const unsigned char *data, size_t length);
	void Delete();

	inline unsigned int ID() { return m_bufferId; }

  private:
	unsigned int m_bufferId = 0;
};
} // namespace sowa

#endif // SW_AUDIO_STREAM_HPP_