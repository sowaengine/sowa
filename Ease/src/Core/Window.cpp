#include "Core/Window.hpp"
#include "raylib.h"
#include "Ease.hpp"
#include <iostream>

#if defined(PLATFORM_DESKTOP)
    #define GLSL_VERSION        330
#else   // PLATFORM_RPI, PLATFORM_ANDROID, PLATFORM_WEB
    #define GLSL_VERSION        100
#endif

#include "rlImGui/rlImGui.h"
#include "Core/Application.hpp"
#include "rlgl.h"
#include "Core/Input.hpp"
#include "ECS/Components/Common/Common.hpp"
#include "ECS/Entity/Entity.hpp"

namespace Ease
{
   Window::Window() {}
   Window::~Window()
   {
      UnloadRenderTexture(m_Target);
      UnloadRenderTexture(m_MousePickingTarget);
      CloseWindow();
   }

   
   void Window::Begin()
   {
      BeginTextureMode(m_Target);
      ClearBackground((Color){30, 30, 30, 255});
   }
   
   void Window::End()
   {
      float scale = MIN((float)GetScreenWidth()/m_VideoWidth, (float)GetScreenHeight()/m_VideoHeight);
      
      EndTextureMode();


      BeginDrawing();
      ClearBackground(BLUE);
      DrawTexturePro(m_Target.texture,
         (Rectangle){ 0.0f, 0.0f, (float)m_Target.texture.width, (float)-m_Target.texture.height },
         (Rectangle){ (GetScreenWidth() - ((float)m_VideoWidth*scale))*0.5f, (GetScreenHeight() - ((float)m_VideoHeight*scale))*0.5f,
         (float)m_VideoWidth*scale, (float)m_VideoHeight*scale }, (Vector2){ 0, 0 }, 0.0f, WHITE);


         static ImFont* pDefaultFont = ImGui::GetIO().Fonts->AddFontFromFileTTF("res/Roboto-Medium.ttf", 16.f);
         BeginRLImGui();
         ImGui::PushFont(pDefaultFont);
         Application::get_singleton().Modules_OnImGuiRender();
         ImGui::PopFont();
         EndRLImGui();

         

      EndDrawing();
   }

   void Window::BeginPicking()
   {
      BeginTextureMode(m_MousePickingTarget);
      ClearBackground((Color){0, 0, 0, 255});
   }
   void Window::EndPicking()
   {
      float scale = MIN((float)GetScreenWidth()/m_VideoWidth, (float)GetScreenHeight()/m_VideoHeight);
      
      EndTextureMode();
      {
         Image img = LoadImageFromTexture(m_MousePickingTarget.texture);
         Ease::Application& app = Application::get_singleton();
         Ease::Entity entity;
         uint32_t id = 0x0;
         Ease::Vec2 mouseInImage;
         mouseInImage.x = Input::GetGameMousePos().x;
         mouseInImage.y = GetVideoHeight() - Input::GetGameMousePos().y;
         if(mouseInImage.x > 0 && mouseInImage.x < GetVideoWidth() && mouseInImage.y > 0 && mouseInImage.y < GetVideoHeight())
         {
            Color color = GetImageColor(img, mouseInImage.x, mouseInImage.y);
            id = ((color.r & 0xFF) << 16) + ((color.g & 0xFF) << 8) + ((color.b & 0xFF) << 0);

            Ease::Scene* scene = Ease::Application::get_singleton().GetCurrentScene();
            entity = scene->GetEntityByID(id);
            if(entity.IsValid())
               app.m_MousePickedEntity = Ease::Entity((entt::entity)id, &app.m_pCurrentScene->m_Registry);
         }
         if(!entity.IsValid())
            app.m_MousePickedEntity.SetRegistry(nullptr);

         UnloadImage(img);
      }
   }

   
   void Window::CreateWindow(int width, int height, const std::string& title)
   {
      SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT);
      InitWindow(width, height, title.c_str());

      Image easeIcon = LoadImage("res/icon.png");
      SetWindowIcon(easeIcon);

      m_Target = LoadRenderTexture(m_VideoWidth, m_VideoHeight);
      SetTextureFilter(m_Target.texture, TEXTURE_FILTER_POINT);

      m_MousePickingTarget = LoadRenderTexture(m_VideoWidth, m_VideoHeight);
      SetTextureFilter(m_MousePickingTarget.texture, TEXTURE_FILTER_POINT);
   }
   
   bool Window::ShouldClose()
   {
      return WindowShouldClose() && !IsKeyPressed(KEY_ESCAPE);
   }


#ifndef EASE_EDITOR
   int Window::GetBlackbarWidth()
   {
      float scale = MIN((float)GetScreenWidth()/m_VideoWidth, (float)GetScreenHeight()/m_VideoHeight);
      return (GetScreenWidth() - ((float)m_VideoWidth*scale))*0.5f;
   }
   int Window::GetBlackbarHeight()
   {
      float scale = MIN((float)GetScreenWidth()/m_VideoWidth, (float)GetScreenHeight()/m_VideoHeight);
      return (GetScreenHeight() - ((float)m_VideoHeight*scale))*0.5f;
   }
#else
   int Window::GetBlackbarWidth()
   {
      return m_Editor_BlackbarWidth;
   }
   int Window::GetBlackbarHeight()
   {
      return m_Editor_BlackbarHeight;
   }
#endif
} // namespace Ease