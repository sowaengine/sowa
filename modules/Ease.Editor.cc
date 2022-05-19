/**
 * @file Ease.Editor.cc
 * @author Lexographics
 * @brief Core Editor Module for Ease Engine
 * @version 0.1
 * @date 2022-05-19
 */
#include <iostream>
#include <vector>
#include <string>
#include "../include/Ease.hpp"
#include "../include/dylib.hpp"
#include "imgui-docking/imgui.h"

class EaseEditor : public Ease::BaseModule
{
   public:
      // Panels
      
   public:
      ~EaseEditor()
      {
         std::cout << "Editor Destroyed" << std::endl;
      }

      void Start() override
      {
         std::cout << "Editor Started" << std::endl;
      }

      void Update() override
      {
         
      }

      void OnImGuiRender() override
      {
         ImGui::ShowDemoWindow();

         ImGui::Begin("Test");
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

