#include "Ease.hpp"
#include "Editor/Editor.hpp"

#include <filesystem>

#include "imgui/imgui.h"
#include "Utils/File.hpp"
#include "Scene/SceneTree/Node.hpp"

namespace Ease
{

static std::string m_CurrentFolder = "";
static std::string projectDir;

static float buttonSize = 88.f;


static void drawFolder(const std::string& folderPath, EditorData& editorData)
{
   //ImGui::Text( ("res://"+m_CurrentFolder).c_str());
   //ImGui::TableNextRow();


   bool doubleClicked = ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left);
   bool singleClicked = ImGui::IsMouseClicked(ImGuiMouseButton_Left, false);

// Go parent folder button
   std::filesystem::directory_entry dir(folderPath);
   if(std::filesystem::relative(dir.path(), projectDir).string() != ".")
   {
      ImGui::ImageButton( ImTextureID( editorData.tex_Directory->getTexID() ), {buttonSize, buttonSize});
      if(ImGui::IsItemHovered() && doubleClicked)
      {
         std::string relative = std::filesystem::relative(dir.path().parent_path(), projectDir );
         m_CurrentFolder = relative;
      }
      ImGui::Text("../");
      ImGui::TableNextColumn();
   }

   

   

   ImTextureID icon = 0;
// iterate for directories
   for (const auto& dirEntry : std::filesystem::directory_iterator(folderPath))
   {
      if(dirEntry.is_directory())
      {
         icon = ImTextureID(editorData.tex_Directory->getTexID());
         ImGui::ImageButton( icon, {buttonSize, buttonSize});
         if(ImGui::IsItemHovered())
         {
            if(doubleClicked)
            {
               std::string relative = std::filesystem::relative(dirEntry.path(), projectDir );
               m_CurrentFolder = relative;
            }
         }
         ImGui::TextWrapped(dirEntry.path().filename().string().c_str());
         
         ImGui::TableNextColumn();
      }
   }
// iterate for files
   for (const auto& dirEntry : std::filesystem::directory_iterator(folderPath))
   {
      if(!dirEntry.is_directory())
      {
         icon = ImTextureID(editorData.tex_File->getTexID());
         ImGui::ImageButton( icon, {buttonSize, buttonSize});
         if(ImGui::IsItemHovered())
         {
            if(singleClicked)
            {
               std::string path = std::filesystem::relative(dirEntry.path(), projectDir);

               editorData.selectedItemFilename = path;
               std::cout << editorData.selectedItemFilename << std::endl;
            }
         }
         ImGui::TextWrapped(dirEntry.path().filename().string().c_str());
         
         ImGui::TableNextColumn();
      }
   }

}


static void drawDirectoryRecursive(const std::string& folderPath)
{
   for (const auto& dirEntry : std::filesystem::directory_iterator(folderPath))
   {
      if(dirEntry.is_directory())
      {
         
         bool x = ImGui::CollapsingHeader(dirEntry.path().filename().c_str());
         if( x )
         {
            ImGui::Indent(10.f);
            drawDirectoryRecursive(dirEntry.path());
            ImGui::Unindent(10.f);
            //ImGui::TreePop();
         }
         
      }
   }
   for (const auto& dirEntry : std::filesystem::directory_iterator(folderPath))
   {
      if(!dirEntry.is_directory())
      {
         
         bool x = ImGui::Selectable(dirEntry.path().filename().c_str());
         if( x )
         {
            Editor::printLine("Line");
            std::cout << "Selecting from left filesystem doesnt affect selected item right now" << std::endl;
         }
         
      }
   }
}
static void drawFileSystem()
{
   drawDirectoryRecursive(projectDir);
}

void Editor::drawContentBrowser()
{
   projectDir = File::getPath_Head("res");

   if(ImGui::Begin("Files"))
   {
      if( ImGui::BeginTable("main", 2, ImGuiTableFlags_Resizable) )
      {
         ImGui::TableNextColumn();
         drawFileSystem();
         ImGui::TableNextColumn();


         int regionWidth = ImGui::GetContentRegionAvail().x * 0.9 - 100;
         int offset = 16;
         int columnCount = regionWidth / ((int)buttonSize+offset);  
         if(columnCount < 1) columnCount = 1;
         

         if( ImGui::BeginTable("directory", columnCount) )
         {
            ImGui::TableNextColumn();
            drawFolder(projectDir + m_CurrentFolder, editorData);

            ImGui::EndTable();
         }
         ImGui::EndTable();
      }


   }
   ImGui::End();
}
   
} // namespace Ease
