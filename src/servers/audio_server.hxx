#ifndef SW_AUDIO_SERVER_HXX
#define SW_AUDIO_SERVER_HXX
#pragma once

class AudioServer {
  public:
	static AudioServer &get();

	AudioServer();
	~AudioServer();

  private:
#ifndef SW_WEB
	void *m_device;
	void *m_context;
#endif
};

#endif // SW_AUDIO_SERVER_HXX