#include "audio_server.hpp"

#include "AL/al.h"
#include "AL/alc.h"

#include "debug.hpp"
#include "utils/file.hpp"

namespace sowa {

struct AudioServerData {
	ALCdevice *device;
	ALCcontext *context;
};

#define AL(f, _msg)                                \
	f;                                             \
	do {                                           \
                                                   \
		ALenum error = alGetError();               \
		if (error != AL_NO_ERROR) {                \
			fprintf(stderr, _msg ": %d\n", error); \
		}                                          \
	} while (false);


AudioServer::AudioServer() {
	m_data = new AudioServerData;

	m_data->device = alcOpenDevice(NULL);
	if (!m_data->device) {
		std::cerr << "Failed to open device " << std::endl;
		return;
	}

	m_data->context = alcCreateContext(m_data->device, NULL);
	if (!alcMakeContextCurrent(m_data->context)) {
		std::cerr << "Failed to make context current " << std::endl;
	}

	alListener3f(AL_POSITION, 0.0f, 0.0f, 1.0f);
	alListener3f(AL_VELOCITY, 0, 0, 0);

	ALfloat orientation[6] = {0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f};
	alListenerfv(AL_ORIENTATION, orientation);
}
AudioServer::~AudioServer() {
	delete m_data;
}

} // namespace sowa
