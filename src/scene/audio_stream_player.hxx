#ifndef SW_AUDIO_STREAM_PLAYER_HXX
#define SW_AUDIO_STREAM_PLAYER_HXX
#pragma once

#include "node.hxx"

#include "resource/audio_stream/audio_stream.hxx"
#include "resource/resource.hxx"

class AudioStreamPlayer : public Node {
  public:
	virtual ~AudioStreamPlayer();
	AudioStreamPlayer();

	void play();
	void pause();
	void stop();

	bool is_playing();

	void _start() override;
	void _exit() override;

	inline RID &stream() { return m_stream; }
	inline bool &autoplay() { return m_autoplay; }

  protected:
	unsigned int m_source_id = 0;

	RID m_stream;
	bool m_autoplay = false;

  private:
	uint32_t m_last_buffer = 0;
};

#endif // SW_AUDIO_STREAM_PLAYER_HXX