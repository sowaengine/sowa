#include "Ease.hpp"
#include "Editor/Editor.hpp"

#include "imgui/imgui.h"
#include "ImGuizmo/ImGuizmo.h"
#include "Scene/Components.hpp"

#include "Utils/Matrix.hpp"
#include "glm/gtc/type_ptr.hpp"

namespace Ease
{

void Editor::drawViewport()
{
   if( ImGui::Begin("Viewport", nullptr, ImGuiWindowFlags_NoCollapse) )
   {
      ImVec2 panelSize = ImGui::GetWindowSize();

      // center buttons
      // button count to display on top bar that will be centered
      int buttonCount = 2;
      const int buttonSize = 32;
      int availWidth = ImGui::GetContentRegionAvail().x;

      // 3D - 2D & Gizmo buttons are aligned to left so calculate content region before displaying them
      if( ImGui::Button("2D", ImVec2(buttonSize, buttonSize)) )
      {
         editorData.editorCameraType = EditorCameraType::TWO_D;
      }
      ImGui::SameLine();
      if(ImGui::Button("3D", ImVec2(buttonSize, buttonSize)))
      {
         editorData.editorCameraType = EditorCameraType::THREE_D;
      }
      ImGui::SameLine();
      m_EditorReturnData.editorCameraType = editorData.editorCameraType;


      ImGui::SetCursorPosX( (availWidth/2) - ((buttonCount*buttonSize)/2) ); 
      if( ImGui::ImageButton( ImTextureID( ((uint32_t)editorData.tex_PlayButton->getTexID()) ), ImVec2(buttonSize, buttonSize)) )
      {
         m_EditorReturnData.gameStartResult = EditorGameStartResult::START;
      }
      ImGui::SameLine();
      if( ImGui::ImageButton( ImTextureID( ((uint32_t)editorData.tex_StopButton->getTexID()) ), ImVec2(buttonSize, buttonSize)) )
      {
         m_EditorReturnData.gameStartResult = EditorGameStartResult::STOP;
      }


      static ImGuizmo::OPERATION op = ImGuizmo::OPERATION::TRANSLATE;
            
      if(ImGui::Button("Move"))
         op = ImGuizmo::OPERATION::TRANSLATE;
      ImGui::SameLine();

      if(ImGui::Button("Rotate"))
         op = ImGuizmo::OPERATION::ROTATE;
      ImGui::SameLine();

      if(ImGui::Button("Scale"))
         op = ImGuizmo::OPERATION::SCALE;

      if(glfwGetKey(editorData.window->get(), GLFW_KEY_Q) == GLFW_PRESS)
         op = ImGuizmo::OPERATION::BOUNDS;
      if(glfwGetKey(editorData.window->get(), GLFW_KEY_W) == GLFW_PRESS)
         op = ImGuizmo::OPERATION::TRANSLATE;
      if(glfwGetKey(editorData.window->get(), GLFW_KEY_E) == GLFW_PRESS)
         op = ImGuizmo::OPERATION::ROTATE;
      if(glfwGetKey(editorData.window->get(), GLFW_KEY_R) == GLFW_PRESS)
         op = ImGuizmo::OPERATION::SCALE;



      float startOfViewport = ImGui::GetCursorPosY();

      ImVec2 availSpace = ImGui::GetContentRegionAvail();

      panelSize.x = availSpace.x;
      int w, h;
      editorData.window->getVideoSize(&w, &h);
      panelSize.y = panelSize.x * ((float)h/w);

      if(panelSize.y > availSpace.y)
      {
         panelSize.y = availSpace.y;
         panelSize.x = panelSize.y * ((float)w/h);
      }
      float offsetFromCenter = (availSpace.x - panelSize.x)/2;
      ImGui::SetCursorPosX(offsetFromCenter);
      offsetFromCenter = (availSpace.y - panelSize.y)/2;
      ImGui::SetCursorPosY(offsetFromCenter + startOfViewport);
      
      ImTextureID frameBufferTexture = ImTextureID( ((uint32_t)editorData.finalFrameBuffer->getColorAttachment()));
      ImVec4 viewportBorderColor = ImVec4(0.2f, 0.2f, 1.f, 0.f);
      if(editorData.gameRunning)
      {
         viewportBorderColor.w = 1.f;
      }

      // for borders
      panelSize.x -= 2.f;
      panelSize.y -= 2.f;
      
      ImVec2 viewportStartPos = ImGui::GetCursorPos();
      ImGui::Image(frameBufferTexture, panelSize, ImVec2(0, 0), ImVec2(1, 1), ImVec4(1.f, 1.f, 1.f, 1.f), viewportBorderColor);
      
      // viewport dragging
      static bool isRightClickStarted = false;
      if(ImGui::IsMouseClicked(ImGuiMouseButton_Right) && ImGui::IsItemHovered()) isRightClickStarted = true;
      if(ImGui::IsMouseReleased(ImGuiMouseButton_Right)) isRightClickStarted = false;

      if(isRightClickStarted)
         m_EditorReturnData.viewportDragDelta = ImGui::GetIO().MouseDelta;
      // viewport scrolling
      if(ImGui::IsItemHovered())
      {
         ImVec2 mouseScroll = ImVec2(ImGui::GetIO().MouseWheel, ImGui::GetIO().MouseWheelH);
         m_EditorReturnData.viewportScrollDelta = mouseScroll;
      }

      // Draw Grid
      {
         Transform3DComponent* cameraTc;
         if(editorData.editorCameraType == EditorCameraType::TWO_D)
            cameraTc = editorData.camera2DTransform;
         else
            cameraTc = editorData.camera3DTransform;

         int width=1280, height=720;
         editorData.window->getVideoSize(&width, &height);

         glm::mat4 view{1.f};
         //view = glm::translate(view, {-width*0.5, -height*0.5, 0.f});
         //view = glm::scale(view, cameraTc->scale);
         //view = glm::translate(view, cameraTc->translation*glm::vec3(1.f, -1.f, 1.f) );
         view = cameraTc->getTransform();
         view = glm::inverse(view);
                     
         glm::mat4 proj{1.f};
         if(editorData.editorCameraType == EditorCameraType::TWO_D)
            proj = editorData.editorCamera2D->getProjection();
         else if(editorData.editorCameraType == EditorCameraType::THREE_D)
            proj = editorData.editorCamera3D->getProjection();


         //glm::mat4 floorMatrix = glm::scale(glm::mat4(1.f), {128.f, 128.f, 128.f});
         //ImGuizmo::DrawGrid(glm::value_ptr(view), glm::value_ptr(proj), glm::value_ptr(floorMatrix), 32);
      }

      // Draw Gizmo
      // Only translation for now
      if(editorData.selectedNode != nullptr)
      {
         if(editorData.selectedNode->hasComponent<Transform3DComponent>())
         {
            Transform3DComponent* cameraTc;
            if(editorData.editorCameraType == EditorCameraType::TWO_D)
               cameraTc = editorData.camera2DTransform;
            else
               cameraTc = editorData.camera3DTransform;

            auto& tc = editorData.selectedNode->getComponent<Transform3DComponent>();

            int width=1280, height=720;
            editorData.window->getVideoSize(&width, &height);

            glm::mat4 view{1.f};
            //view = glm::translate(view, {-width*0.5, -height*0.5, 0.f});
            //view = glm::translate(view, cameraTc->translation*glm::vec3(1.f, 1.f, 1.f) );
            //view = glm::scale(view, cameraTc->scale);
            view = cameraTc->getTransform(true, false);
            view = glm::inverse(view);
            
            
            ImGuizmo::SetGizmoSizeClipSpace(editorData.editorCameraType == EditorCameraType::TWO_D ? 0.1f : 16.f);
            ImGuizmo::AllowAxisFlip(false);
            
                        
            glm::mat4 proj{1.f};
            if(editorData.editorCameraType == EditorCameraType::TWO_D)
               proj = editorData.editorCamera2D->getProjection();
            else if(editorData.editorCameraType == EditorCameraType::THREE_D)
               proj = editorData.editorCamera3D->getProjection();

            glm::mat4 transform = tc.getTransform(false, false, true, true, true);
            
            ImGuizmo::SetRect(viewportStartPos.x + ImGui::GetWindowPos().x, viewportStartPos.y + ImGui::GetWindowPos().y, panelSize.x, panelSize.y);
            ImGuizmo::SetOrthographic(editorData.editorCameraType == EditorCameraType::TWO_D); // two_d -> ortho // three_d -> perspective
            ImGuizmo::SetDrawlist();

            
            

            ImGuizmo::Manipulate(glm::value_ptr(view), glm::value_ptr(proj), op,
                  ImGuizmo::MODE::WORLD, glm::value_ptr(transform));
            /*ImGuizmo::Manipulate(glm::value_ptr(view), glm::value_ptr(proj), ImGuizmo::OPERATION::ROTATE_X,
                  ImGuizmo::MODE::WORLD, glm::value_ptr(transform));
            ImGuizmo::Manipulate(glm::value_ptr(view), glm::value_ptr(proj), ImGuizmo::OPERATION::ROTATE_Y,
                  ImGuizmo::MODE::WORLD, glm::value_ptr(transform));
            ImGuizmo::Manipulate(glm::value_ptr(view), glm::value_ptr(proj), ImGuizmo::OPERATION::ROTATE_Z,
                  ImGuizmo::MODE::WORLD, glm::value_ptr(transform));*/
            
            
            if(ImGuizmo::IsUsing())
            {
               glm::vec3 translation, rotation, scale;
               Matrix::decomposeMat4(transform, translation, rotation, scale);

               if(op == ImGuizmo::OPERATION::TRANSLATE)
               {
                  tc.translation = {translation.x, -translation.y, translation.z};
               }
               else if(op == ImGuizmo::OPERATION::ROTATE)
               {
                  // Todo: fix gizmo rotations
                  // maybe later
                  tc.rotation.x = -rotation.x;
                  tc.rotation.y = rotation.y;
                  tc.rotation.z = -rotation.z;
               }
               else if(op == ImGuizmo::OPERATION::SCALE)
               {
                  tc.scale = scale;
               }
                
            }


         }
      }

      
   }
   ImGui::End();
}
   
} // namespace Ease
