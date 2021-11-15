#include "Ease.hpp"
#include "Editor/Editor.hpp"

#include "Scene/Components.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "imgui/imgui.h"
#include "Utils/File.hpp"

#include "Global.hpp"


#define LOG std::cout << __LINE__ << std::endl;

namespace Ease
{

enum class FileType
{
   
};

FileType getType_File(const std::string& filepath)
{
   return FileType();
}

template<typename T, typename UIFunction>
static void drawComponent(const std::string& name, Node* node, UIFunction uiFunc)
{
   if(node == nullptr) { std::cout << "NODE IS NULLPTR" << std::endl; return; }
   auto& component = node->getComponent<T>();
   //ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_SpanFullWidth | ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_NoAutoOpenOnLog;

   ImGui::Separator();
   bool clicked = ImGui::CollapsingHeader(name.c_str(), 0);

   if(clicked)
   {
      ImGui::Indent();
      uiFunc(component);

      if(ImGui::Button( ("Remove " + name).c_str() ))
      {
         node->removeComponent<T>();
      }

      ImGui::Unindent();
   }
   
   
   

}


static void drawNode(Node* node)
{
   
   char name[256] = {0};
   strcpy(name, node->getName().c_str());
   ImGui::Text("%s", "Name"); ImGui::SameLine();
   if(ImGui::InputText("##Name", name, 256, ImGuiInputTextFlags_EnterReturnsTrue))
      node->setName(name);
   

// Draw Components
   // Transform3DComponent Component
   if(node->hasComponent<Transform3DComponent>())
   {
      drawComponent<Transform3DComponent>("Transform3D", node, [](Transform3DComponent& component)
      {
         ImGui::Text("Position"); ImGui::SameLine();
         ImGui::DragFloat3("##Position", glm::value_ptr(component.translation));

      // user sees rotation as degrees
         ImGui::Text("Rotation"); ImGui::SameLine();
         float rotationDeg[3] = { glm::degrees(component.rotation.x), glm::degrees(component.rotation.y), glm::degrees(component.rotation.z)};
         ImGui::DragFloat3("##Rotation", rotationDeg, 0.25f);
         component.rotation = glm::radians( glm::vec3(rotationDeg[0], rotationDeg[1], rotationDeg[2] ));
      
         ImGui::Text("Scale"); ImGui::SameLine();
         ImGui::DragFloat3("##Scale", glm::value_ptr(component.scale));
      });
   }
   // CameraComponent Component
   if(node->hasComponent<CameraComponent>())
   {
      drawComponent<CameraComponent>("CameraComponent", node, [](CameraComponent& component)
      {
         ImGui::Text("Projection"); ImGui::SameLine();
         const char* projTypes[] = {"Orthographic", "Perspective"};

         static int currItem = 0;
         ImGui::Combo("##", &currItem, projTypes, 2);
         component.projectionType = (CameraProjection)currItem;

         ImGui::Text("Current"); ImGui::SameLine();
         ImGui::Checkbox("##Current", &component.current);
         
         ImGui::Text("Near & Far"); ImGui::SameLine();
         ImGui::DragFloatRange2("##label", &component.near, &component.far, 1.f, 0.1f, 0.f, "%.3f", nullptr, ImGuiSliderFlags_AlwaysClamp);
         component.near = component.near < 0.f ? 0.f : component.near;


         if(component.projectionType == CameraProjection::PERSPECTIVE)
         {
            ImGui::Text("Fov"); ImGui::SameLine();
            ImGui::SliderAngle("##Fov", &component.fov, 0.f, 180.f);

            ImGui::Text("Ratio"); ImGui::SameLine();
            ImGui::SliderFloat("##Ratio", &component.aspect, 0.f, 10.f);
         } else
         {
            ImGui::Text("Size"); ImGui::SameLine();
            ImGui::DragFloat2("##Size", &component.size.x);
         }
      });
   }

   ImGui::Spacing();
   ImGui::Separator();
   ImGui::Spacing();ImGui::Spacing();
// Add Component
   if(ImGui::Button("Add Component"))
      ImGui::OpenPopup("_add_comp");
   
   if(ImGui::BeginPopup("_add_comp"))
   {
      if(!node->hasComponent<Transform3DComponent>())
         if(ImGui::MenuItem("Transform3DComponent"))
            node->addComponent<Transform3DComponent>();

      if(!node->hasComponent<CameraComponent>())
         if(ImGui::MenuItem("CameraComponent"))
            node->addComponent<CameraComponent>();

      ImGui::EndPopup();
   }
}

void Editor::drawInspector()
{
   
   if(ImGui::Begin("Inspector"))
   {
      if(editorData.selectedNode != nullptr)
      {
         drawNode(editorData.selectedNode);
      }

      ImGui::End();
   }
}
   
} // namespace Ease
