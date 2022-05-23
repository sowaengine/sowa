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
#include "rlImGui/rlImGui.h"
#include <memory>
#include <unordered_map>
#include "../src/Resource/ResourceManager.h"
#include "../src/Resource/Texture/Texture.h"
#include "../src/Resource/EditorTheme/EditorTheme.h"

class EaseEditor : public Ease::BaseModule
{
   public:
      std::string m_EditorStatusText{"Ease Engine v" EASE_VERSION_STRING};

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

      // Temporary!
      void SetTheme(const std::string& theme)
      {
         // Todo: make 'Theme' class that can be applied to editor directly ( class will be on engine so other modules can use it )
         static std::unordered_map<std::string, int> builtinThemes = {
            {"Dark", 1},
            {"Light", 2},
            {"Classic", 3}
         };
         if(builtinThemes[theme] == builtinThemes["Dark"])
         {
            ImGui::StyleColorsDark();
         }
         else if(builtinThemes[theme] == builtinThemes["Light"])
         {
            ImGui::StyleColorsLight();
         }
         else if(builtinThemes[theme] == builtinThemes["Classic"])
         {
            ImGui::StyleColorsClassic();
         }
      }

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
            Ease::Window& window = Ease::Application::get_singleton().GetWindow();
            const Texture* image = &window.GetFinalFramebufferTexture().texture;
            Rectangle srcRect{};
               srcRect.x = 0.f;
               srcRect.y = (float)image->height;
               srcRect.width = (float)image->width;
               srcRect.height = -(float)image->height;
               
            int dstWidth = ImGui::GetContentRegionAvail().x;
            int dstHeight = ImGui::GetContentRegionAvail().y;
            float aspect = (float)window.GetWindowWidth() / (float)window.GetWindowHeight();
            if(ImGui::GetContentRegionAvail().x * aspect > ImGui::GetContentRegionAvail().y 
              && ImGui::GetContentRegionAvail().y * aspect < ImGui::GetContentRegionAvail().x)
            {
               dstWidth = ImGui::GetContentRegionAvail().y * aspect;
               dstHeight = ImGui::GetContentRegionAvail().y;

               float regionAvail = ImGui::GetContentRegionAvail().x;
               ImGui::SetCursorPosX(((regionAvail - dstWidth) / 2) + ImGui::GetCursorStartPos().x);
            }
            else
            {
               dstWidth = ImGui::GetContentRegionAvail().x;
               dstHeight = ImGui::GetContentRegionAvail().x / aspect;
               
               float regionAvail = ImGui::GetContentRegionAvail().y;
               ImGui::SetCursorPosY(((regionAvail - dstHeight) / 2.f) + ImGui::GetCursorStartPos().y);
            }
            
            RLImGuiImageRect(
               image,
               dstWidth, dstHeight,
               srcRect
            );
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


         Ease::ResourceManager<Ease::EditorTheme> themeLoader = Ease::ResourceManager<Ease::EditorTheme>::GetLoader();

         std::shared_ptr<Ease::EditorTheme> theme = themeLoader.LoadResource("res/theme.escfg");
         ImGui::GetStyle() = theme->GetStyle();
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

         static bool modal_about_ease = false;
         static bool modal_editor_settings = false;

         /** <Menu Bar> **/
         if(ImGui::BeginMainMenuBar())
         {

            if(ImGui::BeginMenu("View"))
            {
               ImGui::EndMenu();
            }

            if(ImGui::BeginMenu("Editor"))
            {
               if(ImGui::MenuItem("Editor Settings"))
                  modal_editor_settings = true;

               ImGui::EndMenu();
            }

            if(ImGui::BeginMenu("Help"))
            {
               if(ImGui::MenuItem("About Ease"))
                  modal_about_ease = true;

               ImGui::EndMenu();
            }

            if(ImGui::MenuItem("Export theme"))
            {
               std::shared_ptr<Ease::EditorTheme> theme = std::make_shared<Ease::EditorTheme>();
               theme->LoadFromStyle(ImGui::GetStyle());
               Ease::ResourceManager<Ease::EditorTheme>::GetLoader().SaveResource("res/theme_new.escfg", theme);
            }


            ImGui::EndMainMenuBar();
         }

         /** </Menu Bar> **/
         ImGui::DockSpace(ImGui::GetID("Dockspace"), ImVec2(0.f, 0.f), ImGuiDockNodeFlags_PassthruCentralNode);

         ImGui::ShowDemoWindow();
         for(Panel& panel : panels)
         {
            ImGui::Begin(panel.title.c_str(), nullptr, ImGuiWindowFlags_NoCollapse);
               panel.drawFunc();
            ImGui::End();
         }

         const ImGuiViewport* viewport = ImGui::GetMainViewport();
         ImGui::SetNextWindowPos(
            ImVec2(viewport->Pos.x,
                  viewport->Pos.y + viewport->Size.y - ImGui::GetFrameHeight())
         );
         ImGui::SetNextWindowSize(ImVec2(viewport->Size.x, ImGui::GetFrameHeight()));
         ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0);
         ImGui::PushStyleColor(ImGuiCol_MenuBarBg,     ImVec4(0.17f, 0.50f, 0.70f, 1.f)); // (45, 128, 178)
         ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.20f, 0.55f, 0.75f, 1.f)); // (51, 140, 191)
         if(ImGui::Begin("##statusbar", nullptr, 
              ImGuiWindowFlags_NoCollapse
            | ImGuiWindowFlags_NoDecoration
            | ImGuiWindowFlags_NoDocking
            | ImGuiWindowFlags_NoMove
            | ImGuiWindowFlags_NoResize
            | ImGuiWindowFlags_NoSavedSettings
            | ImGuiWindowFlags_MenuBar
            | ImGuiWindowFlags_NoScrollbar
            | ImGuiWindowFlags_NoScrollWithMouse))
         {
            ImGui::PopStyleVar();

            if(ImGui::BeginMenuBar())
            {
               ImGui::MenuItem("This is a Button");
               ImGui::MenuItem("This is a Button too");
               ImGui::MenuItem("This is a Button too too");

               ImGui::SameLine(ImGui::GetWindowContentRegionMax().x - (ImGui::CalcTextSize(m_EditorStatusText.c_str()).x * 1.2f) );
               ImGui::Text("%s", m_EditorStatusText.c_str());

               ImGui::EndMenuBar();
            }
            ImGui::End();

            ImGui::PopStyleColor(2);
         } else { ImGui::PopStyleVar(); ImGui::PopStyleColor(2); }
         
         ImGui::End();



         if(modal_editor_settings)
         {
            Ease::Window& window = Ease::Application::get_singleton().GetWindow();
            ImGui::SetNextWindowSize(ImVec2(
               (window.GetWindowWidth() / 2.5f),
               (window.GetWindowHeight() / 2.f)
            ));
            ImGui::PushStyleVar(ImGuiStyleVar_WindowTitleAlign, ImVec2(0.5f, 0.5f));
            if(ImGui::Begin("Editor Settings", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse))
            {
               static int selected_tab = 0;
               
               const int TAB_INTERFACE = 0;
               const int TAB_THEME     = 1;
   
               if(ImGui::Button("Interface"))
                  selected_tab = TAB_INTERFACE;
               
               ImGui::SameLine();
               if(ImGui::Button("Theme"))
                  selected_tab = TAB_THEME;
               

               if(selected_tab == TAB_INTERFACE)
               {
                  ImGui::Text("Inteface items");
                  ImGui::Text("Inteface items");
                  ImGui::Text("Inteface items");
                  ImGui::Text("Inteface items");
               }

               if(selected_tab == TAB_THEME)
               {
                  static std::string selected_preset = "Dark";

                  ImGui::Text("Preset"); ImGui::SameLine();
                  
                  if(ImGui::BeginCombo("##Preset", selected_preset.c_str()))
                  {
                     if(ImGui::Selectable("Dark"))
                     {
                        selected_preset = "Dark";
                        SetTheme(selected_preset);
                     }
                     if(ImGui::Selectable("Light"))
                     {
                        selected_preset = "Light";
                        SetTheme(selected_preset);
                     }
                     if(ImGui::Selectable("Classic"))
                     {
                        selected_preset = "Classic";
                        SetTheme(selected_preset);
                     }

                     ImGui::EndCombo();
                  }
                  //if(ImGui::Combo("##Presets", &theme_preset_index, presets_str.c_str()))
                     //std::cout << "Set theme to " << theme_preset_index << std::endl;
               }


               ImGui::SetCursorPosX((ImGui::GetWindowSize().x / 2) - (ImGui::CalcTextSize("Close").x * (3.f / 2.f)));
               if (ImGui::Button( "Close", ImVec2(
                     ImGui::CalcTextSize("Close").x * 3,
                     ImGui::CalcTextSize("Close").y * 1.5)))
               {
                  modal_editor_settings = false;
               }
               ImGui::SetWindowPos(ImVec2(
                  (window.GetWindowWidth() / 2) - (ImGui::GetWindowSize().x / 2),
                  (window.GetWindowHeight() / 2) - (ImGui::GetWindowSize().y / 2)
               ));
            }
            ImGui::PopStyleVar();
            ImGui::End();
         }

         if(modal_about_ease)
         {
            static std::shared_ptr<Ease::Texture> iconTexture = Ease::ResourceManager<Ease::Texture>::GetLoader().LoadResource("res/icon.png");
         
            Ease::Window& window = Ease::Application::get_singleton().GetWindow();
            ImGui::SetNextWindowSize(ImVec2(
               (window.GetWindowWidth() / 2.5f),
               (window.GetWindowHeight() / 2.f)
            ));
            ImGui::PushStyleVar(ImGuiStyleVar_WindowTitleAlign, ImVec2(0.5f, 0.5f));
            if(ImGui::Begin("About Ease", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse))
            {
               ImGui::SetCursorPosX((ImGui::GetWindowSize().x / 2) - 64);
               RLImGuiImageSize(&iconTexture->GetTexture(), 128, 128);

               ImGui::SetCursorPosX((ImGui::GetWindowSize().x / 2) - (ImGui::CalcTextSize("Ease Engine").x / 2));
               ImGui::Text("Ease Engine");

               ImGui::NewLine();
               ImGui::SetCursorPosX((ImGui::GetWindowSize().x / 2) - (ImGui::CalcTextSize("Lexographics").x / 2));
               ImGui::Text("Lexographics");


               ImGui::SetCursorPosX((ImGui::GetWindowSize().x / 2) - (ImGui::CalcTextSize("Close").x * (3.f / 2.f)));
               if (ImGui::Button( "Close", ImVec2(
                     ImGui::CalcTextSize("Close").x * 3,
                     ImGui::CalcTextSize("Close").y * 1.5)))
               {
                  modal_about_ease = false;
               }
               ImGui::SetWindowPos(ImVec2(
                  (window.GetWindowWidth() / 2) - (ImGui::GetWindowSize().x / 2),
                  (window.GetWindowHeight() / 2) - (ImGui::GetWindowSize().y / 2)
               ));
            }
            ImGui::PopStyleVar();
            ImGui::End();
         }
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

