#include "audio_server.hxx"

#include "AL/al.h"
#include "AL/alc.h"

#include "utils/utils.hxx"

AudioServer &AudioServer::get() {
	static AudioServer *server = new AudioServer;
	return *server;
}

AudioServer::AudioServer() {
#ifndef SW_WEB
	m_device = alcOpenDevice(NULL);
	if (!m_device) {
		Utils::Error("Failed to open audio device: {}", alGetError());
		return;
	}

	m_context = alcCreateContext(reinterpret_cast<ALCdevice *>(m_device), NULL);
	if (!alcMakeContextCurrent(reinterpret_cast<ALCcontext *>(m_context))) {
		Utils::Error("Failed to bind audio context: {}", alGetError());
		return;
	}

	ALfloat listenerOri[] = {0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f};
	alListener3f(AL_POSITION, 0, 0, 1.0f);
	alListener3f(AL_VELOCITY, 0, 0, 0);
	alListenerfv(AL_ORIENTATION, listenerOri);

	Utils::Info("Audio server initialized");
#endif
}
AudioServer::~AudioServer() {
}
