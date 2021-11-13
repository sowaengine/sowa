#include "Ease.hpp"
#include "Editor/Editor.hpp"

#include "Scene/Components.hpp"

#include "imgui/imgui.h"
#include "Utils/File.hpp"

#include "Global.hpp"


namespace Ease
{

static std::string s_ConsoleText = "";

// static
void Editor::printLine(const std::string& line)
{
   s_ConsoleText += line + "\n";
}

void Editor::drawConsole()
{
   
   if(ImGui::Begin("Console"))
   {
      
      ImGui::TextWrapped("%s", s_ConsoleText.c_str());

      ImGui::SetCursorPosY(ImGui::GetWindowSize().y-32+ImGui::GetScrollY());
      static char text[512] = {0};
      ImGui::Text("%s", "Enter Command: "); ImGui::SameLine();
      if(ImGui::InputText("##Enter Text", text, 512, ImGuiInputTextFlags_EnterReturnsTrue))
      {
         Editor::printLine(text);
         memset(text, 0, 512);
      }
   }
   ImGui::End();
}
   
} // namespace Ease
