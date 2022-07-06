#include "AudioStreamPlayer.hpp"

namespace Ease::Component
{
   AudioStreamPlayer::AudioStreamPlayer()
   {   

   }

   AudioStreamPlayer::~AudioStreamPlayer()
   {

   }

   void AudioStreamPlayer::Play()
   {
      m_Source.Play(m_Stream);
   }
}