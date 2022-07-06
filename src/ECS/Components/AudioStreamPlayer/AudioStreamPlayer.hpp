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


        std::shared_ptr<Ease::AudioStream>& Stream() { return m_Stream; }

        bool& Looping() { return m_Source.Looping(); }
        void Play();

        void UpdateData() { m_Source.UpdateData(); }
    private:
        SoundSource m_Source;

        std::shared_ptr<Ease::AudioStream> m_Stream;
    };
}

#endif