#include "Window.h"
#include "raylib.h"
#include "Ease.hpp"
#include <iostream>

#if defined(PLATFORM_DESKTOP)
    #define GLSL_VERSION        330
#else   // PLATFORM_RPI, PLATFORM_ANDROID, PLATFORM_WEB
    #define GLSL_VERSION        100
#endif

#include "rlImGui/rlImGui.h"
#include "Application.h"


namespace Ease
{
   Window::Window() {}
   Window::~Window()
   {
      UnloadRenderTexture(m_Target);
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
      ClearBackground(BLACK);
      DrawTexturePro(m_Target.texture,
         (Rectangle){ 0.0f, 0.0f, (float)m_Target.texture.width, (float)-m_Target.texture.height },
         (Rectangle){ (GetScreenWidth() - ((float)m_VideoWidth*scale))*0.5f, (GetScreenHeight() - ((float)m_VideoHeight*scale))*0.5f,
         (float)m_VideoWidth*scale, (float)m_VideoHeight*scale }, (Vector2){ 0, 0 }, 0.0f, WHITE);
      
         BeginRLImGui();
         ImGui::ShowDemoWindow();
         
         Application::get_singleton().Modules_OnImGuiRender();
         EndRLImGui();

         

      EndDrawing();
   }

   
   void Window::CreateWindow(int width, int height, const std::string& title)
   {
      SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT);
      InitWindow(width, height, title.c_str());

      Image easeIcon = LoadImage("res/icon.png");
      SetWindowIcon(easeIcon);

      m_Target = LoadRenderTexture(m_VideoWidth, m_VideoHeight);
      SetTextureFilter(m_Target.texture, TEXTURE_FILTER_POINT);
   }
   
   bool Window::ShouldClose()
   {
      return WindowShouldClose();
   }
} // namespace Ease