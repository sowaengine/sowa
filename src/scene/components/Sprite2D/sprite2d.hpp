#ifndef __COMP_SPRITE_2D_HPP__
#define __COMP_SPRITE_2D_HPP__

#include "resource/resourceManager.hpp"

namespace Ease
{
   namespace Comp
   {
      typedef struct Sprite2D {
         Texture* texture;

         Sprite2D();
         ~Sprite2D();
      } Sprite2D;
   }
}

#endif // __COMP_SPRITE_2D_HPP__