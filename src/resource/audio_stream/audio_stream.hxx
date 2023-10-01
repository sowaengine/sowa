#ifndef SW_AUDIO_STREAM_HXX
#define SW_AUDIO_STREAM_HXX
#pragma once

#include <stdint.h>

#include "core/error/error.hxx"

class AudioStream {
  public:
	AudioStream() = default;
	~AudioStream();

	ErrorCode Load(const char *path);

	void Delete();

	inline uint32_t ID() const { return m_id; }

  private:
	uint32_t m_id = 0;
};

#endif // SW_AUDIO_STREAM_HXX