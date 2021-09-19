#include "sprite2d.hpp"
#include <iostream>
#include <assert.h>

namespace Ease
{
   namespace Comp
   {
      Sprite2D::Sprite2D()
         : texture(nullptr)
      {

      }

      Sprite2D::~Sprite2D()
      {

      }


      void Sprite2D::Draw()
      {
         std::cout << "Drawed Sprite2D" << std::endl;
         assert(false && "This method is not used. Don't use it");
      }
   }
}