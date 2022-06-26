#include "Renderer.hpp"
#include "Resource/Texture/Texture.hpp"

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

   void Renderer::DrawQuadColor(const glm::vec2& center, const glm::vec2& size, float rotation, Color color, const glm::vec2& offset)
   {
      rlPushMatrix();
         rlTranslatef(center.x, -center.y, 0.f);
         rlRotatef(rotation, 0, 0, 1);
         rlTranslatef(-size.x / 2.f, -size.y / 2.f, 0.f);
         rlTranslatef(offset.x, offset.y, 0.f);
         
         DrawRectangle(0, 0, size.x, size.y, color);
         //DrawRectanglePro(rect, {size.x / 2.f, size.y / 2.f}, rotation, color);
      rlPopMatrix();
   }
   void Renderer::DrawCircleColor(const glm::vec2& center, float radius, float rotation, Color color, const glm::vec2& offset)
   {
      rlPushMatrix();
         rlTranslatef(center.x, -center.y, 0.f);
         rlRotatef(rotation, 0, 0, 1);
         rlTranslatef(offset.x, offset.y, 0.f);
         DrawCircle(0, 0, radius, color);
      rlPopMatrix();
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