#ifndef _E_AUDIOSTREAM_HPP__
#define _E_AUDIOSTREAM_HPP__

#pragma once

#include <stdint.h>
#include <string>
#include <AL/al.h>
#include "../Resource.hpp"

typedef uint32_t ResourceID;

namespace Ease
{

class AudioStream : public BaseResource
{
public:
    AudioStream();
    ~AudioStream();

    const std::string& GetFilepath() { return m_Filepath; }
private:
    template<typename> friend class ResourceManager;
    friend class SoundSource;

    std::string m_Filepath{""};
    ALuint m_Buffer;
};

}
#endif