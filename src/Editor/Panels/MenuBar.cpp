#include "Ease.hpp"
#include "Editor/Editor.hpp"

#include "imgui/imgui.h"


namespace Ease
{
void Editor::drawMenuBar()
{
   // Menu
   if (ImGui::BeginMenuBar())
   {
      if (ImGui::BeginMenu("Scene"))
      {
         // Scene > New Scene
         if( ImGui::MenuItem("New Scene", NULL, false) )
         {
            std::cout << "New Scene" << std::endl;
         }
         // Scene > Open Scene (from project directory)
         if( ImGui::MenuItem("Open Scene", NULL, false) )
         {
            std::cout << "Open Scene" << std::endl;
         }
         // Scene > Reopen Closed Scene
         if( ImGui::MenuItem("Reopen Closed Scene", "Control+Shift+T", false) )
         {
            std::cout << "Reopen Closed Scene" << std::endl;
         }
         // Scene > separator1
         ImGui::Separator();

         // Scene > Save Scene
         if( ImGui::MenuItem("Save Scene", "Control+S", false) )
         {
            std::cout << "Save Scene" << std::endl;
         }
         // Scene > Save Scene As
         if( ImGui::MenuItem("Save Scene As", "Control+Shift+S", false) )
         {
            std::cout << "Save Scene As" << std::endl;
         }
         // Scene > Save All Scenes
         if( ImGui::MenuItem("Save All Scenes", "Control+Shift+Alt+S", false) )
         {
            std::cout << "Save All Scenes" << std::endl;
         }

         // Scene > separator2
         ImGui::Separator();

         // Scene > Undo
         if( ImGui::MenuItem("Undo", "Control+Z", false) )
         {
            std::cout << "Undo" << std::endl;
         }
         // Scene > Redo
         if( ImGui::MenuItem("Redo", "Control+Shift+Z", false) )
         {
            std::cout << "Redo" << std::endl;
         }

         // Scene > separator3
         ImGui::Separator();

         // Scene > Redo
         if( ImGui::MenuItem("Close Scene", "Control+Shift+W", false) )
         {
            std::cout << "Close Scene" << std::endl;
         }

         // Scene > separator4
         ImGui::Separator();

         if( ImGui::MenuItem("Quit", NULL, false) )
         {
            std::cout << "Quit" << std::endl;
         }
         
         ImGui::EndMenu();
      }
      if (ImGui::BeginMenu("Project"))
      {
         // Project > Project Settings
         if( ImGui::MenuItem("Project Settings", NULL, false) )
         {
            std::cout << "Project Settings" << std::endl;
         }
         
         // Project > separator1
         ImGui::Separator();

         // Project > Export
         if( ImGui::MenuItem("Export", NULL, false) )
         {
            std::cout << "Export" << std::endl;
         }

         // Project > separator2
         ImGui::Separator();

         // Project > Quit to Project List
         if( ImGui::MenuItem("Quit to Project List", NULL, false) )
         {
            std::cout << "Quit to Project List" << std::endl;
         }

         ImGui::EndMenu();
      }
      if (ImGui::BeginMenu("Editor"))
      {
         // Editor > Editor Settings
         if( ImGui::MenuItem("Editor Settings", NULL, false) )
         {
            std::cout << "Editor Settings" << std::endl;
         }
         
         ImGui::EndMenu();
      }
      ImGui::EndMenuBar();
   }
}

} // namespace Ease
