#ifndef _E_SOUNDDEVICE_HPP__
#define _E_SOUNDDEVICE_HPP__
#pragma once

#include <vector>
#include <AL/al.h>
#include <AL/alc.h>
#include <stddef.h>

namespace Ease
{
    class SoundDevice
    {
        public:
            static SoundDevice& get_singleton()
            {
                static SoundDevice device;
                return device;
            }

            ALuint LoadSoundBuffer(unsigned char* data, size_t size);
            ALuint LoadSoundBufferFromFile(const char* path);
            bool UnloadSoundBuffer(ALuint buffer);
        private:
            SoundDevice();
            ~SoundDevice();

            ALCdevice* pALCDevice = nullptr;
            ALCcontext* pALCContext = nullptr;

            std::vector<ALuint> m_SoundBuffers;
    };
}
#endif