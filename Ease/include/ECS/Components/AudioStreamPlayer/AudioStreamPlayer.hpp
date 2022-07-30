#ifndef _E_AUDIOSTREAMPLAYER_HPP__
#define _E_AUDIOSTREAMPLAYER_HPP__

#pragma once
#include <stdint.h>
#include "Core/Sound/SoundSource.hpp"

namespace Ease::Component
{
    class AudioStreamPlayer
    {
    public:
        AudioStreamPlayer();
        ~AudioStreamPlayer();


        ResourceID& Stream() { return m_Stream; }
        std::shared_ptr<Ease::AudioStream>& GetStream();

        bool& Looping() { return m_Source.Looping(); }
        void Play();
        void Stop();

        void UpdateData() { m_Source.UpdateData(); }
    private:
        SoundSource m_Source;

        ResourceID m_Stream;
    };
}

#endif