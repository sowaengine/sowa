#include "Renderer.h"
#include "Resource/Texture/Texture.h"

#include "raylib.h"
#include "rlgl.h"
#include <vector>

namespace Ease
{
   void Renderer::DrawQuad(const glm::vec2& position, const glm::vec2& scale, float zIndex, float rotation, Texture& texture, const glm::vec2& uv1, const glm::vec2& uv2)
   {
      Rectangle rect = (Rectangle) {
               (float)texture.GetTexture().width * uv1.x,
               (float)texture.GetTexture().height * uv1.y,
               
               (float)texture.GetTexture().width * (uv2.x - uv1.x),
               (float)texture.GetTexture().height * (uv2.y - uv1.y)};


      Rectangle dest = (Rectangle) {position.x , -position.y, (float)texture.GetTexture().width * scale.x * (uv2.x - uv1.x), (float)texture.GetTexture().height * scale.y * (uv2.y - uv1.y)};
      Vector2 origin = (Vector2){(float)texture.GetTexture().width / 2 * scale.x * (uv2.x - uv1.x), (float)texture.GetTexture().height / 2 * scale.y * (uv2.y - uv1.y)};
      
      
      DrawTexturePro(texture.GetTexture(), rect, dest, origin, rotation, WHITE);
   }
   
   
   
   void Renderer::DrawText(const glm::vec2& position, const glm::vec2& scale, float zIndex, float rotation, float fontSize, const std::string& text, const glm::vec4& color)
   {
      Color c = (Color){(unsigned char)color.r, (unsigned char)color.g, (unsigned char)color.b, (unsigned char)color.a};
      static Font defaultFont = LoadFont("res/Roboto-Medium.ttf");
      

      DrawTextPro(defaultFont, text.c_str(), (Vector2){position.x, -position.y}, (Vector2){0, 0}, rotation, fontSize, 3.f, c);
   }

   
   Renderer::Renderer()
   {
   }

   Renderer::~Renderer()
   {
   }

} // namespace Ease