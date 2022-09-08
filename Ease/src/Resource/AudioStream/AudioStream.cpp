#include "Resource/AudioStream/AudioStream.hpp"
#include "Core/Sound/SoundDevice.hpp"
#include "Resource/ResourceLoader.hpp"

namespace Ease
{
   template<>
   Reference<AudioStream> ResourceLoaderImpl<AudioStream>::LoadFromFile(const char* path)
   {
      Reference<Ease::AudioStream> audio = std::make_shared<Ease::AudioStream>();
      
      SoundDevice& device = SoundDevice::get_singleton();
      audio->m_Buffer = device.LoadSoundBufferFromFile(path);

      return audio;
   }
   AudioStream::AudioStream()
   {

   }

   AudioStream::~AudioStream()
   {
      Ease::SoundDevice::get_singleton().UnloadSoundBuffer(m_Buffer);
   }
} // namespace Ease