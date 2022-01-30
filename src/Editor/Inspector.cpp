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
   std::string headerID = entity.GetName() + "_" + name; // entity15_sprite
   ImGui::PushID(headerID.c_str());
   bool clicked = ImGui::CollapsingHeader(name.c_str(), 0);
   ImGui::PopID();
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

      if(m_EditorData->inspectorTarget == EditorData::InspectorTarget::ENTITY)
      {
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
                  if(!selectedEntity.hasComponent<LuaScriptComponent>() && ImGui::MenuItem("LuaScript"))
                  {
                     selectedEntity.addComponent<LuaScriptComponent>();
                  }
                  if(!selectedEntity.hasComponent<SpriteComponent>() && ImGui::MenuItem("Sprite"))
                  {
                     selectedEntity.addComponent<SpriteComponent>();
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
               if(ImGui::Selectable("Script"))
                  Editor::get_singleton().SetSelectedResource(component.script.get());
            });
            drawComponent<SpriteComponent>("Sprite", selectedEntity, [](SpriteComponent& component)
            {
               if(ImGui::Selectable("Texture"))
                  Editor::get_singleton().SetSelectedResource(component.textureRes.get());
            });
            
         }
      }
      else if(m_EditorData->inspectorTarget == EditorData::InspectorTarget::RESOURCE)
      {
         if(m_EditorData->selectedResource != nullptr)
         {
            std::vector<ResourceProperty*>& props = m_EditorData->selectedResource->properties;
            
            for(ResourceProperty* prop : props)
            {
               ImGui::Text("%s", prop->name.c_str());
               if(prop->type == PropertyType::SCRIPT || prop->type == PropertyType::TEXTURE)
               {
                  ImGui::Button("##prop", ImVec2(ImGui::GetContentRegionAvail().x, 20));
                  if(ImGui::IsItemHovered())
                  {
                     ImGui::BeginTooltip();
                     ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
                     ImGui::TextUnformatted(prop->dataStr.c_str());
                     ImGui::PopTextWrapPos();
                     ImGui::EndTooltip();
                  }

                  if(ImGui::BeginDragDropTarget())
                  {
                     const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("__ASSETS_ITEM_FILE_PATH__");
                     if(payload != nullptr)
                     {
                        const char* path = (const char*)payload->Data;
                        prop->dataStr = path;
                        m_EditorData->selectedResource->PropertiesUpdated();
                     }

                     ImGui::EndDragDropTarget();
                  }
               }
            }
         }
      }

      ImGui::End();
   }
}