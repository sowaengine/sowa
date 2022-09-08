#include "Resource/SpriteSheetAnimation/SpriteSheetAnimation.hpp"
#include "Resource/ResourceLoader.hpp"

namespace Ease
{
   template<>
   Reference<SpriteSheetAnimation> ResourceLoaderImpl<SpriteSheetAnimation>::Load(unsigned char* data, size_t size)
   {
      Reference<SpriteSheetAnimation> animation = std::make_shared<SpriteSheetAnimation>();
      return animation;
   }
   SpriteSheetAnimation::SpriteSheetAnimation()
   {

   }

   SpriteSheetAnimation::~SpriteSheetAnimation()
   {
      
   }
} // namespace Ease