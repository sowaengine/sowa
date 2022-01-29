/**
 * @file ScenePanel.cpp
 * @author Lexographics
 * @brief 
 * @version 0.1
 * @date 2022-01-29
 */
#include "Editor.hpp"
#include "imgui.h"

void Editor::UpdateScenePanel()
{
   if(ImGui::Begin("Scene"))
   {
      auto view = m_EditorData->currentScene->GetRegistry().view<CommonComponent>();
      for(auto it = view.rbegin(); it != view.rend(); ++it)
      {
         Entity entity(*it, &m_EditorData->currentScene->GetRegistry());

         CommonComponent common = entity.getComponent<CommonComponent>();

         ImGuiSelectableFlags flags = 0;
      
         ImVec4 tintColor = ImVec4(1.f, 1.f, 1.f, 1.f);
         if(entity.hasComponent<CameraComponent>() && entity.getComponent<CameraComponent>().current)
            tintColor = ImVec4(0.2f, 0.5f, 1.f, 1.f);

         ImGui::Image(ImTextureID((GLuint64)m_EntityIconTexture), ImVec2(16.f, 16.f), ImVec2(0.f, 0.f), ImVec2(1.f, 1.f), tintColor); ImGui::SameLine();
         if(ImGui::Selectable(common.name.c_str(), false, flags))
         //if(ImGui::CollapsingHeader(common.name.c_str(), flags))
         {
            m_EditorData->selectedEntity.SetEntityID(*it);
            m_EditorData->selectedEntity.SetRegistry(&m_EditorData->currentScene->GetRegistry());

            //ImGui::TreePop();
         }
      }

      ImGui::End();
   }
}