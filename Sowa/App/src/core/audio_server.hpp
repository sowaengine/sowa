#ifndef SW_AUDIO_SERVER_HPP_
#define SW_AUDIO_SERVER_HPP_

#include "sowa.hpp"


namespace sowa {
struct AudioServerData;
class AudioServer {
  public:
  private:
	friend class Application;
	AudioServer();
	~AudioServer();

    AudioServerData* m_data = nullptr;
};

} // namespace sowa

#endif // SW_AUDIO_SERVER_HPP_