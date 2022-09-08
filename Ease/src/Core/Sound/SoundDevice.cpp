#include "Core/Sound/SoundDevice.hpp"
#include <AL/al.h>

#include <stdexcept>
#include <exception>
#include <cerrno>
#include <iostream>

#include "Core/Application.hpp"

#include <sndfile.h>
#include <AL/alext.h>
#include "Core/Sound/SoundDevice.hpp"
#include <cassert>

namespace Ease
{
    SoundDevice::SoundDevice()
    {       
        pALCDevice = alcOpenDevice(nullptr);
        assert(pALCDevice && "Failed to open ALCDevice");

        pALCContext = alcCreateContext(pALCDevice, nullptr);
        assert(pALCContext && "Failed to create ALCContext");
        
        assert(alcMakeContextCurrent(pALCContext) && "Failed alcMakeContextCurrent");
        
        const char* name = nullptr;
        if(alcIsExtensionPresent(pALCDevice, "ALC_ENUMERATE_ALL_EXT"))
            name = alcGetString(pALCDevice, ALC_ALL_DEVICES_SPECIFIER);
        if(!name || alcGetError(pALCDevice) != AL_NO_ERROR)
            name = alcGetString(pALCDevice, ALC_DEVICE_SPECIFIER);

        Application::get_singleton().Log(std::string("Loaded Device: ") + name);
    }

    SoundDevice::~SoundDevice()
    {
        alDeleteBuffers(m_SoundBuffers.size(), m_SoundBuffers.data());
        m_SoundBuffers.clear();
        
        alcMakeContextCurrent(nullptr);
        alcDestroyContext(pALCContext);
        alcCloseDevice(pALCDevice);
    }
        
    ALuint SoundDevice::LoadSoundBuffer(unsigned char* data, size_t size)
    {
        ALenum err, format;
        ALuint buffer;
        SNDFILE* sndFile;
        SF_INFO sfinfo;
        short* membuf;
        sf_count_t num_frames;
        ALsizei num_bytes;
        
        assert(false && "Can not load sound buffer from memory");
        sndFile = sf_open("path", SFM_READ, &sfinfo);
        if(!sndFile)
        {
            std::cout << "path" << " :: " << sf_strerror(sndFile) << std::endl;
            throw std::runtime_error("Cannot load audio file");
        }
        if(sfinfo.frames < 1 || sfinfo.frames > (sf_count_t)(INT_MAX / sizeof(short)) / sfinfo.channels)
        {
            std::cout << "Bad sample count: " << "path" << std::endl;
            sf_close(sndFile);
            throw("");
        }

        format = AL_NONE;
        if(sfinfo.channels == 1)
            format = AL_FORMAT_MONO16;
        else if(sfinfo.channels == 2)
            format = AL_FORMAT_STEREO16;
        else if(sfinfo.channels == 3)
            if(sf_command(sndFile, SFC_WAVEX_GET_AMBISONIC, NULL, 0) == SF_AMBISONIC_B_FORMAT)
                format = AL_FORMAT_BFORMAT2D_16;
        else if(sfinfo.channels == 4)
            if(sf_command(sndFile, SFC_WAVEX_GET_AMBISONIC, NULL, 0) == SF_AMBISONIC_B_FORMAT)
                format = AL_FORMAT_BFORMAT3D_16;
        
        if(!format)
        {
            std::cout << "Bad channel count: " << sfinfo.channels << ", Path: " << "path" << std::endl;
            sf_close(sndFile);
            throw("");
        }
        membuf = static_cast<short*>(malloc((size_t)(sfinfo.frames * sfinfo.channels) * sizeof(short)));
        num_frames = sf_readf_short(sndFile, membuf, sfinfo.frames);
        if(num_frames < 1)
        {
            free(membuf);
            sf_close(sndFile);
            std::cout << "Failed to read samples : " << "path" << std::endl;
            throw("");
        }
        num_bytes = (ALsizei)(num_frames * sfinfo.channels) * (ALsizei)sizeof(short);

        buffer = 0;
        alGenBuffers(1, &buffer);
        alBufferData(buffer, format, membuf, num_bytes, sfinfo.samplerate);
        
        free(membuf);
        sf_close(sndFile);

        err = alGetError();
        if(err != AL_NO_ERROR)
        {
            std::cout << "OpenAL ERROR : " << alGetString(err) << std::endl;
            if(buffer && alIsBuffer(buffer))
                alDeleteBuffers(1, &buffer);
            throw("");
        }
        return buffer;
    }

    ALuint SoundDevice::LoadSoundBufferFromFile(const char* path)
    {
        ALenum err, format;
        ALuint buffer;
        SNDFILE* sndFile;
        SF_INFO sfinfo;
        short* membuf;
        sf_count_t num_frames;
        ALsizei num_bytes;
        
        sndFile = sf_open(path, SFM_READ, &sfinfo);
        if(!sndFile)
        {
            std::cout << path << " :: " << sf_strerror(sndFile) << std::endl;
            throw std::runtime_error("Cannot load audio file");
        }
        if(sfinfo.frames < 1 || sfinfo.frames > (sf_count_t)(INT_MAX / sizeof(short)) / sfinfo.channels)
        {
            std::cout << "Cannot load audio file. Bad sample count" << std::endl;
            sf_close(sndFile);
            throw("");
        }

        format = AL_NONE;
        if(sfinfo.channels == 1)
            format = AL_FORMAT_MONO16;
        else if(sfinfo.channels == 2)
            format = AL_FORMAT_STEREO16;
        else if(sfinfo.channels == 3)
            if(sf_command(sndFile, SFC_WAVEX_GET_AMBISONIC, NULL, 0) == SF_AMBISONIC_B_FORMAT)
                format = AL_FORMAT_BFORMAT2D_16;
        else if(sfinfo.channels == 4)
            if(sf_command(sndFile, SFC_WAVEX_GET_AMBISONIC, NULL, 0) == SF_AMBISONIC_B_FORMAT)
                format = AL_FORMAT_BFORMAT3D_16;
        
        if(!format)
        {
            std::cout << "Cannot load audio file. Bad channel count" << sfinfo.channels << std::endl;
            sf_close(sndFile);
            throw("");
        }
        membuf = static_cast<short*>(malloc((size_t)(sfinfo.frames * sfinfo.channels) * sizeof(short)));
        num_frames = sf_readf_short(sndFile, membuf, sfinfo.frames);
        if(num_frames < 1)
        {
            free(membuf);
            sf_close(sndFile);
            std::cout << "Cannot load audio file. Failed to read samples" << std::endl;
            throw("");
        }
        num_bytes = (ALsizei)(num_frames * sfinfo.channels) * (ALsizei)sizeof(short);

        buffer = 0;
        alGenBuffers(1, &buffer);
        alBufferData(buffer, format, membuf, num_bytes, sfinfo.samplerate);
        
        free(membuf);
        sf_close(sndFile);

        err = alGetError();
        if(err != AL_NO_ERROR)
        {
            std::cout << "OpenAL ERROR : " << alGetString(err) << std::endl;
            if(buffer && alIsBuffer(buffer))
                alDeleteBuffers(1, &buffer);
            throw("");
        }
        return buffer;
    }

    bool SoundDevice::UnloadSoundBuffer(ALuint buffer)
    {
        for(auto it = m_SoundBuffers.begin(); it != m_SoundBuffers.end(); ++it)
        {
            if(*it == buffer)
            {
                alDeleteBuffers(1, &*it);
                m_SoundBuffers.erase(it);
                return true;
            }
        }
        return false;
    }
} // namespace Ease
