/**
 * @file Ease.Editor.cc
 * @author Lexographics
 * @brief Core Editor Module for Ease Engine
 * @version 0.1
 * @date 2022-05-19
 */
#include <iostream>
#include <string>
#include <vector>
#include <functional>
#include "../include/Ease.hpp"
#include "../include/dylib.hpp"
#include "imgui-docking/imgui.h"
#include "../src/Core/Application.h"
#include "../src/Core/EditorLayer.h"
#include "rlImGui/rlImGui.h"

class EaseEditor : public Ease::BaseModule
{
   public:
      struct Panel
      {
         bool visible = false;
         std::string title;
         std::function<void()> drawFunc;

         Panel(const char* _title, std::function<void()> _drawFunc)
            : title(_title), drawFunc(_drawFunc)
         {
            
         }
      };
      std::vector<Panel> panels;

   public:
      ~EaseEditor()
      {
         std::cout << "Editor Destroyed" << std::endl;
      }

      struct {
         static void empty()
         {
            ImGui::Text("Empty");
         }
         static void scene()
         {
            RLImGuiImageSize(
               &Ease::Application::get_singleton().GetWindow().GetFinalFramebufferTexture().texture,
               ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y);
         }
      } draw;

      void Start() override
      {
         //ImGui::SetCurrentContext(Ease::Application::GetImGuiContext());

         panels.emplace_back("Hierarchy", draw.empty);
         panels.emplace_back("Scene"    , draw.scene);
         panels.emplace_back("Inspector", draw.empty);
         panels.emplace_back("Project"  , draw.empty);
         panels.emplace_back("Console"  , draw.empty);
      }

      void Update() override
      {
         static int i = 0;
         i++;
      }

      void OnImGuiRender() override
      {
         Ease::Window& window = Ease::Application::get_singleton().GetWindow();

         ImGui::SetNextWindowPos(ImVec2(0.f, 0.f));
         ImGui::SetNextWindowSize(ImVec2(window.GetWindowWidth(), window.GetWindowHeight()));
         ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.f, 0.f));
         ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0);
         ImGui::Begin("Editor", nullptr,
              ImGuiWindowFlags_NoBringToFrontOnFocus
            | ImGuiWindowFlags_NoNavFocus
            | ImGuiWindowFlags_NoDocking
            | ImGuiWindowFlags_NoTitleBar
            | ImGuiWindowFlags_NoResize
            | ImGuiWindowFlags_NoMove
            | ImGuiWindowFlags_NoCollapse
            | ImGuiWindowFlags_MenuBar
            | ImGuiWindowFlags_NoBackground);
         ImGui::PopStyleVar(2);
         ImGui::DockSpace(ImGui::GetID("Dockspace"), ImVec2(0.f, 0.f), ImGuiDockNodeFlags_PassthruCentralNode);
         //ImGui::ShowDemoWindow();

         for(Panel& panel : panels)
         {
            ImGui::Begin(panel.title.c_str());
               panel.drawFunc();
            ImGui::End();
         }
         
         ImGui::End();
      }

};



DYLIB_API Ease::BaseModule* Create()
{
   EaseEditor* lib = new EaseEditor;
   lib->metadata.authorName = "Ease";
   lib->metadata.moduleName = "Editor";
   lib->metadata.version = 1;
   return lib;
}

DYLIB_API void Destroy(Ease::BaseModule* lib)
{
   delete reinterpret_cast<EaseEditor*>(lib);
}

