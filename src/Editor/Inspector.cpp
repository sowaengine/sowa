/**
 * @file Inspector.cpp
 * @author Lexographics
 * @brief 
 * @version 0.1
 * @date 2022-01-29
 */
#include "Editor.hpp"
#include "imgui.h"

template<typename T, typename UIFunction>
static void drawComponent(const std::string& name, Entity& entity, UIFunction uiFunc)
{
   if(!entity.hasComponent<T>())
      return;
   auto& component = entity.getComponent<T>();
   
   //ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_SpanFullWidth | ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_NoAutoOpenOnLog;

   ImGui::Separator();
   bool clicked = ImGui::CollapsingHeader(name.c_str(), 0);
   if(clicked)
   {
      ImGui::Indent();
      uiFunc(component);

      if(ImGui::Button( ("Remove " + name).c_str() ))
      {
         entity.removeComponent<T>();
      }

      ImGui::Unindent();
   }
}

void Editor::UpdateInspector()
{
   if(ImGui::Begin("Inspector"))
   {
      // Edit Selected Entity
      if(m_EditorData->selectedEntity.IsValid())
      {
         Entity selectedEntity = m_EditorData->selectedEntity;

         auto& commonComponent = selectedEntity.getComponent<CommonComponent>();
         
         
         char nameBuf[512];
         memset(nameBuf, 0, 512);
         strcpy(nameBuf, commonComponent.name.data());
         ImGui::Text("%s", "Name: "); ImGui::SameLine();
         if(ImGui::InputText("##name", nameBuf, 512, ImGuiInputTextFlags_EnterReturnsTrue))
         {
            commonComponent.name = nameBuf;
            if(commonComponent.name.size() == 0)
               commonComponent.name = "Unnamed";
         }


         // Adding component with right click
         if(ImGui::IsMouseClicked(ImGuiMouseButton_Right) && ImGui::IsWindowHovered())
            ImGui::OpenPopup("INSPECTOR-RCLICK");
         
         if(ImGui::BeginPopup("INSPECTOR-RCLICK"))
         {
            if(ImGui::BeginMenu("Add Component"))
            {
               if(!selectedEntity.hasComponent<Transform2DComponent>() && ImGui::MenuItem("Transform 2D"))
               {
                  selectedEntity.addComponent<Transform2DComponent>();
               }
               if(!selectedEntity.hasComponent<CameraComponent>() && ImGui::MenuItem("Camera"))
               {
                  selectedEntity.addComponent<CameraComponent>();
               }
               ImGui::EndMenu();
            }

            ImGui::EndPopup();
         }

         // Draw Components              // Maybe set ids like name+id so all components have different ids, not necessary for now
         drawComponent<Transform2DComponent>("Transform 2D", selectedEntity, [](Transform2DComponent& component)
         {
            ImGui::DragFloat2("Pos", &component.position.x);
            ImGui::DragFloat2("Scale", &component.scale.x);

            float deg = glm::degrees(component.rotation);
            ImGui::DragFloat("Rotation", &deg);
            component.rotation = glm::radians(deg);

            if(ImGui::SliderInt("Z Index", &component.zIndex, 0, 1000))
               Editor::get_singleton().Log << "Z Index Not Implemented" << Log::Endl;
         });
         drawComponent<CameraComponent>("Camera", selectedEntity, [](CameraComponent& component)
         {
            ImGui::Checkbox("Current", &component.current);
         });
         drawComponent<LuaScriptComponent>("LuaScript", selectedEntity, [](LuaScriptComponent& component)
         {
            
         });
         
      }
      

      ImGui::End();
   }
}