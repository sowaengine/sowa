/**
 * @file Viewport.cpp
 * @author Lexographics
 * @brief 
 * @version 0.1
 * @date 2022-01-26
 */
#include "Editor.hpp"
#include "imgui.h"
#include "Application/Application.hpp"

void Editor::UpdateViewport()
{
   if(ImGui::Begin("Viewport"))
   {
      ImTextureID playButtonTexture = (void*)1;
      if(Application::get_singleton().IsRunning())
         playButtonTexture = (void*)(GLuint64)m_StopTexture;
      else
         playButtonTexture = (void*)(GLuint64)m_PlayTexture;

      if(ImGui::ImageButton(ImTextureID(playButtonTexture), ImVec2(24, 24)))
      {
         m_EditorData->start_stop_pressed = true;
      }

      ImGui::SameLine();
      ImGui::Checkbox("In Game Camera", &m_EditorData->useInGameCamera);


      ImVec2 regionAvail = ImGui::GetContentRegionAvail();
      regionAvail.x = regionAvail.y * 16.f / 9.f;
      ImGui::Image(ImTextureID((GLuint64)m_EditorData->finalFramebufferTextureID), regionAvail);


      static ImVec2 mouseDragStart;
      static bool dragging = false;
      if (ImGui::IsMouseClicked(ImGuiMouseButton_Right) && ImGui::IsWindowHovered())
      {
         mouseDragStart = ImGui::GetMousePos();
         dragging = true;
      }
      if(!ImGui::IsMouseDown(ImGuiMouseButton_Right))
      {
         dragging = false;
         m_EditorData->viewportDragMouseDeltaX = 0;
         m_EditorData->viewportDragMouseDeltaY = 0;
      }
      
      
      if(dragging)
      {
         m_EditorData->viewportDragMouseDeltaX = mouseDragStart.x - ImGui::GetMousePos().x;
         m_EditorData->viewportDragMouseDeltaY = mouseDragStart.y - ImGui::GetMousePos().y;
         mouseDragStart = ImGui::GetMousePos();
      }
      

      ImGui::End();
   }
}