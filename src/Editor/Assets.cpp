/**
 * @file Assets.cpp
 * @author Lexographics
 * @brief 
 * @version 0.1
 * @date 2022-01-29
 */
#include "Editor.hpp"
#include "imgui.h"
#include <filesystem>
#include "Resource/Project.hpp"


struct ClickData
{
   bool doubleClicked = false;
   bool clicked = false;
};

ClickData Editor::DrawFolder(const std::string& name)
{
   ClickData data;

   ImGui::ImageButton(ImTextureID((GLuint64)AssetsPanelTextures.m_Directory), ImVec2(64, 64));
   if(ImGui::IsItemHovered())
   {
      data.doubleClicked = ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left);
      data.clicked = ImGui::IsMouseClicked(ImGuiMouseButton_Left);
   }
   ImGui::Text("%s", name.c_str());
   ImGui::TableNextColumn();

   return data;
}
ClickData Editor::DrawFile(const std::string& name)
{
   ClickData data;
   
   GLuint64 image = AssetsPanelTextures.m_UnknownFile;
   std::string extension = std::filesystem::path(name).extension();
   if(extension == ".png" || extension == ".jpg" || extension == ".bmp")
      image = AssetsPanelTextures.m_Image;
   if(extension == ".txt" || extension == ".glsl")
      image = AssetsPanelTextures.m_Text;
   if(extension == ".lua")
      image = AssetsPanelTextures.m_Lua;
   if(extension == ".ttf")
      image = AssetsPanelTextures.m_Font;

   ImGui::ImageButton(ImTextureID(image), ImVec2(64, 64));
   if(ImGui::IsItemHovered())
   {
      data.doubleClicked = ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left);
      data.clicked = ImGui::IsMouseClicked(ImGuiMouseButton_Left);
   }
   ImGui::Text("%s", name.c_str());
   ImGui::TableNextColumn();

   return data;
}


std::filesystem::path currentPath;
// Recursive function
void Editor::DrawItem(std::filesystem::path& currentPath)
{
   // First draw directories, then files
   for(const auto& dirEntry : std::filesystem::directory_iterator(currentPath))
   {
      if(dirEntry.is_directory())
      {
         ClickData data = DrawFolder(dirEntry.path().filename().string());
         if(data.doubleClicked)
            currentPath = dirEntry.path();
      }
   }
   for(const auto& dirEntry : std::filesystem::directory_iterator(currentPath))
   {
      if(!dirEntry.is_directory())
      {
         DrawFile(dirEntry.path().filename().string());
      }
   }
   
}

void Editor::UpdateAssets()
{
   static std::filesystem::path workingFolder = Project::get_singleton().GetAbsolutePath("");
   static std::filesystem::path currentPath = workingFolder;

   if(ImGui::Begin("Assets"))
   {
      if(ImGui::BeginTable("main", 4))
      {
         ImGui::TableNextColumn();

         if(std::filesystem::relative(currentPath, workingFolder) != ".")
         {
            ClickData data = DrawFolder("../");
            if(data.doubleClicked)
               currentPath = currentPath.parent_path();
         }
            
         DrawItem(currentPath);


         ImGui::EndTable();
      }
   }
   ImGui::End();
}