#include "Core/Sound/SoundSource.hpp"

namespace Ease
{
    SoundSource::SoundSource()
    {
        alGenSources(1, &m_Source);
        UpdateData();
    }
    SoundSource::~SoundSource()
    {
        alDeleteSources(1, &m_Source);
    }

    void SoundSource::Play(std::shared_ptr<Ease::AudioStream> stream)
    {
        if(stream->m_Buffer != m_Buffer)
        {
            m_Buffer = stream->m_Buffer;
            alSourcei(m_Source, AL_BUFFER, (ALint)m_Buffer);
        }

        alSourcePlay(m_Source);
    }
    void SoundSource::Stop(std::shared_ptr<Ease::AudioStream> stream)
    {
        if(stream->m_Buffer != m_Buffer)
        {
            m_Buffer = stream->m_Buffer;
            alSourcei(m_Source, AL_BUFFER, (ALint)m_Buffer);
        }

        alSourceStop(m_Source);
    }

    void SoundSource::UpdateData()
    {
        alSourcef(m_Source, AL_PITCH, m_Pitch);
        alSourcef(m_Source, AL_GAIN, m_Gain);
        alSource3f(m_Source, AL_POSITION, m_Pos.x, m_Pos.y, m_Pos.z);
        alSource3f(m_Source, AL_VELOCITY, m_Velocity.x, m_Velocity.y, m_Velocity.z);
        alSourcei(m_Source, AL_LOOPING, m_Loop);
        alSourcei(m_Source, AL_BUFFER, m_Buffer);
    }
} // namespace Ease
