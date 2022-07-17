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

   
   void Renderer::DrawQuadWithID(const glm::vec2& position, const glm::vec2& scale, float zIndex, float rotation, Texture& texture, uint32_t id, const glm::vec2& uv1, const glm::vec2& uv2)
   {
      Rectangle rect = (Rectangle) {
               (float)texture.GetTexture().width * uv1.x,
               (float)texture.GetTexture().height * uv1.y,
               
               (float)texture.GetTexture().width * (uv2.x - uv1.x),
               (float)texture.GetTexture().height * (uv2.y - uv1.y)};


      Rectangle dest = (Rectangle) {position.x , -position.y, (float)texture.GetTexture().width * scale.x * (uv2.x - uv1.x), (float)texture.GetTexture().height * scale.y * (uv2.y - uv1.y)};
      Vector2 origin = (Vector2){(float)texture.GetTexture().width / 2 * scale.x * (uv2.x - uv1.x), (float)texture.GetTexture().height / 2 * scale.y * (uv2.y - uv1.y)};
      
      
      float vec[4] = {0};
      vec[0] = ((id >> 16) & 0xFF) / 255.f;
      vec[1] = ((id >>  8) & 0xFF) / 255.f;
      vec[2] = ((id >>  0) & 0xFF) / 255.f;
      vec[3] = 1.f;
      BeginShaderMode(m_PickingShader);
         SetShaderValue(m_PickingShader, GetShaderLocation(m_PickingShader, "id"), vec, SHADER_UNIFORM_VEC4);
         DrawTexturePro(texture.GetTexture(), rect, dest, origin, rotation, WHITE);
      EndShaderMode();
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
   void Renderer::DrawTextWithID(const glm::vec2& position, const glm::vec2& scale, float zIndex, float rotation, float fontSize, const std::string& text, const glm::vec4& color, uint32_t id)
   {
      Color c = (Color){(unsigned char)color.r, (unsigned char)color.g, (unsigned char)color.b, (unsigned char)color.a};
      static Font defaultFont = LoadFont("res/Roboto-Medium.ttf");
      
      
      float vec[4] = {0};
      vec[0] = ((id >> 16) & 0xFF) / 255.f;
      vec[1] = ((id >>  8) & 0xFF) / 255.f;
      vec[2] = ((id >>  0) & 0xFF) / 255.f;
      vec[3] = 1.f;

      
      BeginShaderMode(m_PickingShader);
         SetShaderValue(m_PickingShader, GetShaderLocation(m_PickingShader, "id"), vec, SHADER_UNIFORM_VEC4);
         DrawTextPro(defaultFont, text.c_str(), (Vector2){position.x, -position.y}, (Vector2){0, 0}, rotation, fontSize, 3.f, c);
      EndShaderMode();
   }

   void Renderer::DrawTextAligned(const glm::vec2& position, const glm::vec2& size, const glm::vec2& alignment, float zIndex, float rotation, float fontSize, const std::string& text, const glm::vec4& color)
   {
      Color c = (Color){(unsigned char)color.r, (unsigned char)color.g, (unsigned char)color.b, (unsigned char)color.a};
      static Font defaultFont = LoadFont("res/Roboto-Medium.ttf");
      
      Vector2 textSize = MeasureTextEx(defaultFont, text.c_str(), fontSize, 0);
      Vector2 offset = {0.f, 0.f};
      offset.x = (size.x - textSize.x) * alignment.x;
      offset.y = (size.y - textSize.y) * alignment.y;

      rlPushMatrix();
         rlTranslatef(position.x, position.y, 0.f);
         rlRotatef(rotation, 0, 0, 1);
         rlTranslatef(offset.x, offset.y, 0.f);
         DrawTextPro(defaultFont, text.c_str(), {0, 0}, (Vector2){0, 0}, 0, fontSize, 3.f, c);
      rlPopMatrix();
   }

   void Renderer::DrawNinePatchRect(const glm::vec2& position, const glm::vec2& size, const glm::vec2& scale, float zIndex, float rotation, Ease::Component::NinePatchRect& npatch)
   {
      NPatchInfo info;
      info.left = npatch.Left();
      info.right = npatch.Right();
      info.top = npatch.Top();
      info.bottom = npatch.Bottom();
      info.layout = NPATCH_NINE_PATCH;
      
      if(auto tex = npatch.Texture().lock(); tex)
      {
         Rectangle srcRect;
         srcRect.x = 0;
         srcRect.y = 0;
         srcRect.width = tex->GetTexture().width;
         srcRect.height = tex->GetTexture().height;
         info.source = srcRect;

         Rectangle dstRect;
         dstRect.x = 0;
         dstRect.y = 0;
         dstRect.width = size.x * scale.y;
         dstRect.height = size.y * scale.y;

         rlPushMatrix();
            rlTranslatef(position.x, position.y, 0.f);
            rlRotatef(rotation, 0, 0, 1);
            DrawTextureNPatch(npatch.Texture().lock()->GetTexture(), info, dstRect, (Vector2){0, 0}, 0, WHITE);
         rlPopMatrix();
      }

   }

   void Renderer::DrawNinePatchRectWithID(const glm::vec2& position, const glm::vec2& size, const glm::vec2& scale, float zIndex, float rotation, Ease::Component::NinePatchRect& npatch, uint32_t id)
   {
      NPatchInfo info;
      info.left = npatch.Left();
      info.right = npatch.Right();
      info.top = npatch.Top();
      info.bottom = npatch.Bottom();
      info.layout = NPATCH_NINE_PATCH;

      float vec[4] = {0};
      vec[0] = ((id >> 16) & 0xFF) / 255.f;
      vec[1] = ((id >>  8) & 0xFF) / 255.f;
      vec[2] = ((id >>  0) & 0xFF) / 255.f;
      vec[3] = 1.f;
      
      if(auto tex = npatch.Texture().lock(); tex)
      {
         Rectangle srcRect;
         srcRect.x = 0;
         srcRect.y = 0;
         srcRect.width = tex->GetTexture().width;
         srcRect.height = tex->GetTexture().height;
         info.source = srcRect;

         Rectangle dstRect;
         dstRect.x = 0;
         dstRect.y = 0;
         dstRect.width = size.x * scale.y;
         dstRect.height = size.y * scale.y;

         BeginShaderMode(m_PickingShader);
            rlPushMatrix();
               rlTranslatef(position.x, position.y, 0.f);
               rlRotatef(rotation, 0, 0, 1);
               
               SetShaderValue(m_PickingShader, GetShaderLocation(m_PickingShader, "id"), vec, SHADER_UNIFORM_VEC4);
               DrawTextureNPatch(npatch.Texture().lock()->GetTexture(), info, dstRect, (Vector2){0, 0}, 0, WHITE);
            rlPopMatrix();
         EndShaderMode();
      }
   }

   
   Renderer::Renderer()
   {
      m_PickingShader = LoadShader("", "res/picking.frag");
   }

   Renderer::~Renderer()
   {
      UnloadShader(m_PickingShader);
   }

} // namespace Ease