#include "AudioStream.hpp"
#include "Core/Sound/SoundDevice.hpp"

namespace Ease
{
   AudioStream::AudioStream()
   {

   }

   AudioStream::~AudioStream()
   {
      Ease::SoundDevice::get_singleton().UnloadSoundBuffer(m_Buffer);
   }
} // namespace Ease