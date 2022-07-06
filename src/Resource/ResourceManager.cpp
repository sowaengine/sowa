#include "ResourceManager.hpp"

#include "Resource/Texture/Texture.hpp"
#include "raylib.h"
#include "dylib.hpp"
#include "yaml-cpp/yaml.h"
#include <fstream>
#include "Utils/YAML.hpp"
#include <iostream>
#include "Core/Sound/SoundDevice.hpp"
#include "Core/ProjectSettings.hpp"
#include "Utils/File.hpp"

namespace Ease
{
   // Resource specific implementations

   /* ==============<Ease::AudioStream>============== */
   template<>
   std::shared_ptr<Ease::AudioStream> ResourceManager<Ease::AudioStream>::LoadResource(const char* path, ResourceID id)
   {
      static ResourceID resCounter = 0;

      std::shared_ptr<Ease::AudioStream> audio = std::make_shared<Ease::AudioStream>();

      ResourceID resID = id != 0 ? id : resCounter++;
      m_Resources[resID] = audio;
      std::filesystem::path fullpath = Ease::File::Path(path);
      audio->SetResourceID(resID);
      audio->m_Filepath = path;
      
      SoundDevice& device = SoundDevice::get_singleton();
      audio->m_Buffer = device.LoadSoundBuffer(fullpath.c_str());

      return audio;
   }

   template<>
   ResourceManager<Ease::AudioStream>& ResourceManager<Ease::AudioStream>::GetLoader()
   {
      static ResourceManager<Ease::AudioStream> loader;

      return loader;
   }
   /* ==============</Ease::AudioStream>============== */

   /* ==============<Ease::Texture>============== */
   template<>
   std::shared_ptr<Ease::Texture> ResourceManager<Ease::Texture>::LoadResource(const char* path, ResourceID id)
   {
      static ResourceID resCounter = 0;

      std::shared_ptr<Ease::Texture> tex = std::make_shared<Ease::Texture>();

      ResourceID resID = id != 0 ? id : resCounter++;
      m_Resources[resID] = tex;
      std::filesystem::path fullpath = Ease::File::Path(path);
      tex->SetTexture(LoadTexture(fullpath.c_str()));
      tex->SetResourceID(resID);
      tex->m_Filepath = path;

      return tex;
   }

   template<>
   ResourceManager<Ease::Texture>& ResourceManager<Ease::Texture>::GetLoader()
   {
      static ResourceManager<Ease::Texture> loader;

      return loader;
   }
   /* ==============</Ease::Texture>============== */
   
   /* ==============<Ease::NativeModule>============== */
   template<>
   std::shared_ptr<Ease::NativeModule> ResourceManager<Ease::NativeModule>::LoadResource(const char* path, ResourceID id)
   {
      static ResourceID resCounter = 0;

      std::string modulePath = Ease::File::Path(path);
      modulePath += dylib::extension;

      dylib lib;
      lib.open(modulePath.c_str());
      
      auto createFunc = lib.get_function<BaseModule*()>("Create");
      auto deleteFunc = lib.get_function<void(BaseModule*)>("Destroy");

      BaseModule* myModule = createFunc();

      std::shared_ptr<Ease::NativeModule> nativeModule = std::make_shared<Ease::NativeModule>();
      nativeModule->SetModule(myModule);
      nativeModule->SetDeleteFunc(deleteFunc);

      ResourceID resID = id != 0 ? id : resCounter++;
      m_Resources[resID] = nativeModule;
      nativeModule->SetResourceID(resID);

      return nativeModule;
   }

   template<>
   ResourceManager<Ease::NativeModule>& ResourceManager<Ease::NativeModule>::GetLoader()
   {
      static ResourceManager<Ease::NativeModule> moduleLoader;

      return moduleLoader;
   }
   /* ==============</Ease::NativeModule>============== */

   /* ==============<Ease::SpriteSheetAnimation>============== */
   template<>
   std::shared_ptr<Ease::SpriteSheetAnimation> ResourceManager<Ease::SpriteSheetAnimation>::LoadResource(const char* path, ResourceID id)
   {
      static ResourceID resCounter = 0;

      std::shared_ptr<Ease::SpriteSheetAnimation> anim = std::make_shared<Ease::SpriteSheetAnimation>();

      ResourceID resID = id != 0 ? id : resCounter++;
      m_Resources[resID] = anim;
      anim->SetResourceID(resID);

      return anim;
   }

   template<>
   ResourceManager<Ease::SpriteSheetAnimation>& ResourceManager<Ease::SpriteSheetAnimation>::GetLoader()
   {
      static ResourceManager<Ease::SpriteSheetAnimation> animLoader;

      return animLoader;
   }
   /* ==============</Ease::SpriteSheetAnimation>============== */

   /* ==============<Ease::EditorTheme>============== */
   template<>
   std::shared_ptr<Ease::EditorTheme> ResourceManager<Ease::EditorTheme>::LoadResource(const char* path, ResourceID id)
   {
      static ResourceID resCounter = 0;

      YAML::Node node = YAML::LoadFile(Ease::File::Path(path));
      if(node["Type"].as<std::string>() != "EditorTheme")
         return nullptr;
      
      std::shared_ptr<Ease::EditorTheme> themeRes = std::make_shared<Ease::EditorTheme>();
      ImGuiStyle& style = themeRes->style;

      // Default theme : ImGui::StyleColorsClassic();
      YAML::Node theme = node["Theme"];
      style.AntiAliasedLines       = theme["AntiAliasedLines"].as<bool>(true);
      style.AntiAliasedLinesUseTex = theme["AntiAliasedLinesUseTex"].as<bool>(true);
      style.AntiAliasedFill        = theme["AntiAliasedFill"].as<bool>(true);

      style.Alpha             = theme["Alpha"].as<float>(1.f);
      style.DisabledAlpha     = theme["DisabledAlpha"].as<float>(0.6f);
      style.WindowRounding    = theme["WindowRounding"].as<float>(0.f);
      style.WindowBorderSize  = theme["WindowBorderSize"].as<float>(1.f);
      style.ChildRounding     = theme["ChildRounding"].as<float>(0.f);
      style.ChildBorderSize   = theme["ChildBorderSize"].as<float>(1.f);
      style.PopupRounding     = theme["PopupRounding"].as<float>(0.f);
      style.PopupBorderSize   = theme["PopupBorderSize"].as<float>(1.f);
      style.FrameRounding     = theme["FrameRounding"].as<float>(0.f);
      style.FrameBorderSize   = theme["FrameBorderSize"].as<float>(0.f);
      style.IndentSpacing     = theme["IndentSpacing"].as<float>(21.f);
      style.ColumnsMinSpacing = theme["ColumnsMinSpacing"].as<float>(6.f);
      style.ScrollbarSize     = theme["ScrollbarSize"].as<float>(14.f);
      style.ScrollbarRounding = theme["ScrollbarRounding"].as<float>(9.f);
      style.GrabMinSize       = theme["GrabMinSize"].as<float>(10.f);
      style.GrabRounding      = theme["GrabRounding"].as<float>(0.f);
      style.LogSliderDeadzone = theme["LogSliderDeadzone"].as<float>(4.f);
      style.TabRounding       = theme["TabRounding"].as<float>(4.f);
      style.TabBorderSize     = theme["TabBorderSize"].as<float>(0.f);
      style.TabMinWidthForCloseButton   = theme["TabMinWidthForCloseButton"].as<float>(0.f);
      style.MouseCursorScale            = theme["MouseCursorScale"].as<float>(1.f);
      style.CurveTessellationTol        = theme["CurveTessellationTol"].as<float>(1.25f);
      style.CircleTessellationMaxError  = theme["CircleTessellationMaxError"].as<float>(0.3f);

      style.WindowPadding      = theme["WindowPadding"].as<ImVec2>(ImVec2(8, 8));
      style.WindowMinSize      = theme["WindowMinSize"].as<ImVec2>(ImVec2(32, 32));
      style.WindowTitleAlign   = theme["WindowTitleAlign"].as<ImVec2>(ImVec2(0, 0.5f));
      style.FramePadding       = theme["FramePadding"].as<ImVec2>(ImVec2(4, 3));
      style.ItemSpacing        = theme["ItemSpacing"].as<ImVec2>(ImVec2(8, 4));
      style.ItemInnerSpacing   = theme["ItemInnerSpacing"].as<ImVec2>(ImVec2(4, 4));
      style.CellPadding        = theme["ItemInnerSpacing"].as<ImVec2>(ImVec2(4, 2));
      style.TouchExtraPadding  = theme["ItemInnerSpacing"].as<ImVec2>(ImVec2(0, 0));
      style.ButtonTextAlign    = theme["ButtonTextAlign"].as<ImVec2>(ImVec2(0.5f, 0.5f));
      style.SelectableTextAlign    = theme["SelectableTextAlign"].as<ImVec2>(ImVec2(0, 0));
      style.DisplayWindowPadding   = theme["DisplayWindowPadding"].as<ImVec2>(ImVec2(19, 19));
      style.DisplaySafeAreaPadding = theme["DisplaySafeAreaPadding"].as<ImVec2>(ImVec2(3, 3));

      style.Colors[ImGuiCol_Text]                 = theme["Text"].as<ImVec4>(ImVec4(0.9f, 0.9f, 0.9f, 1));
      style.Colors[ImGuiCol_TextDisabled]         = theme["TextDisabled"].as<ImVec4>(ImVec4(0.6f, 0.6f, 0.6f, 1));
      style.Colors[ImGuiCol_WindowBg]             = theme["WindowBg"].as<ImVec4>(ImVec4(0.f, 0.f, 0.f, 0.85f));
      style.Colors[ImGuiCol_ChildBg]              = theme["ChildBg"].as<ImVec4>(ImVec4(0.f, 0.f, 0.f, 0.f));
      style.Colors[ImGuiCol_PopupBg]              = theme["PopupBg"].as<ImVec4>(ImVec4(0.11f, 0.11f, 0.14f, 0.92f));
      style.Colors[ImGuiCol_Border]               = theme["Border"].as<ImVec4>(ImVec4(0.5f, 0.5f, 0.5f, 0.5f));
      style.Colors[ImGuiCol_BorderShadow]         = theme["BorderShadow"].as<ImVec4>(ImVec4(0.f, 0.f, 0.f, 0.f));
      style.Colors[ImGuiCol_FrameBg]              = theme["FrameBg"].as<ImVec4>(ImVec4(0.43f, 0.43f, 0.43f, 0.39f));
      style.Colors[ImGuiCol_FrameBgHovered]       = theme["FrameBgHovered"].as<ImVec4>(ImVec4(0.47f, 0.47f, 0.69f, 0.4f));
      style.Colors[ImGuiCol_FrameBgActive]        = theme["FrameBgActive"].as<ImVec4>(ImVec4(0.42f, 0.41f, 0.64f, 0.69f));
      style.Colors[ImGuiCol_TitleBg]              = theme["TitleBg"].as<ImVec4>(ImVec4(0.27f, 0.27f, 0.54f, 0.83f));
      style.Colors[ImGuiCol_TitleBgActive]        = theme["TitleBgActive"].as<ImVec4>(ImVec4(0.32f, 0.32f, 0.63f, 0.87f));
      style.Colors[ImGuiCol_TitleBgCollapsed]     = theme["TitleBgCollapsed"].as<ImVec4>(ImVec4(0.4f, 0.4f, 0.8f, 0.2f));
      style.Colors[ImGuiCol_MenuBarBg]            = theme["MenuBarBg"].as<ImVec4>(ImVec4(0.4f, 0.4f, 0.55f, 0.8f));
      style.Colors[ImGuiCol_ScrollbarBg]          = theme["ScrollbarBg"].as<ImVec4>(ImVec4(0.2f, 0.25f, 0.3f, 0.6f));
      style.Colors[ImGuiCol_ScrollbarGrab]        = theme["ScrollbarGrab"].as<ImVec4>(ImVec4(0.4f, 0.4f, 0.8f, 0.3f));
      style.Colors[ImGuiCol_ScrollbarGrabHovered] = theme["ScrollbarGrabHovered"].as<ImVec4>(ImVec4(0.4f, 0.4f, 0.8f, 0.4f));
      style.Colors[ImGuiCol_ScrollbarGrabActive]  = theme["ScrollbarGrabActive"].as<ImVec4>(ImVec4(0.41f, 0.39f, 0.8f, 0.6f));
      style.Colors[ImGuiCol_CheckMark]            = theme["CheckMark"].as<ImVec4>(ImVec4(0.9f, 0.9f, 0.9f, 0.5f));
      style.Colors[ImGuiCol_SliderGrab]           = theme["SliderGrab"].as<ImVec4>(ImVec4(1, 1, 1, 0.3f));
      style.Colors[ImGuiCol_SliderGrabActive]     = theme["SliderGrabActive"].as<ImVec4>(ImVec4(0.41f, 0.39f, 0.8f, 0.6f));
      style.Colors[ImGuiCol_Button]               = theme["Button"].as<ImVec4>(ImVec4(0.35f, 0.4f, 0.61f, 0.62f));
      style.Colors[ImGuiCol_ButtonHovered]        = theme["ButtonHovered"].as<ImVec4>(ImVec4(0.4f, 0.48f, 0.71f, 0.79f));
      style.Colors[ImGuiCol_ButtonActive]         = theme["ButtonActive"].as<ImVec4>(ImVec4(0.46f, 0.54f, 0.8f, 1));
      style.Colors[ImGuiCol_Header]               = theme["Header"].as<ImVec4>(ImVec4(0.4f, 0.4f, 0.9f, 0.45f));
      style.Colors[ImGuiCol_HeaderHovered]        = theme["HeaderHovered"].as<ImVec4>(ImVec4(0.45f, 0.45f, 0.9f, 0.8f));
      style.Colors[ImGuiCol_HeaderActive]         = theme["HeaderActive"].as<ImVec4>(ImVec4(0.53f, 0.53f, 0.87f, 0.8f));
      style.Colors[ImGuiCol_Separator]            = theme["Separator"].as<ImVec4>(ImVec4(0.5f, 0.5f, 0.5f, 0.6f));
      style.Colors[ImGuiCol_SeparatorHovered]     = theme["SeparatorHovered"].as<ImVec4>(ImVec4(0.6f, 0.6f, 0.7f, 1));
      style.Colors[ImGuiCol_SeparatorActive]      = theme["SeparatorActive"].as<ImVec4>(ImVec4(0.7f, 0.7f, 0.9f, 1));
      style.Colors[ImGuiCol_ResizeGrip]           = theme["ResizeGrip"].as<ImVec4>(ImVec4(1, 1, 1, 0.1f));
      style.Colors[ImGuiCol_ResizeGripHovered]    = theme["ResizeGripHovered"].as<ImVec4>(ImVec4(0.78f, 0.82f, 1, 0.6f));
      style.Colors[ImGuiCol_ResizeGripActive]     = theme["ResizeGripActive"].as<ImVec4>(ImVec4(0.78f, 0.82f, 1, 0.9f));
      style.Colors[ImGuiCol_Tab]                  = theme["Tab"].as<ImVec4>(ImVec4(0.34f, 0.34f, 0.68f, 0.79f));
      style.Colors[ImGuiCol_TabHovered]           = theme["TabHovered"].as<ImVec4>(ImVec4(0.45f, 0.45f, 0.9f, 0.8f));
      style.Colors[ImGuiCol_TabActive]            = theme["TabActive"].as<ImVec4>(ImVec4(0.4f, 0.4f, 0.72f, 0.85f));
      style.Colors[ImGuiCol_TabUnfocused]         = theme["TabUnfocused"].as<ImVec4>(ImVec4(0.28f, 0.28f, 0.57f, 0.82f));
      style.Colors[ImGuiCol_TabUnfocusedActive]   = theme["TabUnfocusedActive"].as<ImVec4>(ImVec4(0.35f, 0.35f, 0.65f, 0.84f));
      style.Colors[ImGuiCol_DockingPreview]       = theme["DockingPreview"].as<ImVec4>(ImVec4(0.4f, 0.4f, 0.9f, 0.31f));
      style.Colors[ImGuiCol_DockingEmptyBg]       = theme["DockingEmptyBg"].as<ImVec4>(ImVec4(0.2f, 0.2f, 0.2f, 1));
      style.Colors[ImGuiCol_PlotLines]            = theme["PlotLines"].as<ImVec4>(ImVec4(1, 1, 1, 1));
      style.Colors[ImGuiCol_PlotLinesHovered]     = theme["PlotLinesHovered"].as<ImVec4>(ImVec4(0.9f, 0.7f, 0, 1));
      style.Colors[ImGuiCol_PlotHistogram]        = theme["PlotHistogram"].as<ImVec4>(ImVec4(0.9f, 0.7f, 0, 1));
      style.Colors[ImGuiCol_PlotHistogramHovered] = theme["PlotHistogramHovered"].as<ImVec4>(ImVec4(1, 0.6f, 0, 1));
      style.Colors[ImGuiCol_TableHeaderBg]        = theme["TableHeaderBg"].as<ImVec4>(ImVec4(0.27f, 0.27f, 0.38f, 1));
      style.Colors[ImGuiCol_TableBorderStrong]    = theme["TableHeaderBg"].as<ImVec4>(ImVec4(0.31f, 0.31f, 0.45f, 1));
      style.Colors[ImGuiCol_TableBorderLight]     = theme["TableBorderLight"].as<ImVec4>(ImVec4(0.26f, 0.26f, 0.28f, 1));
      style.Colors[ImGuiCol_TableRowBg]           = theme["TableRowBg"].as<ImVec4>(ImVec4(0, 0, 0, 0));
      style.Colors[ImGuiCol_TableRowBgAlt]        = theme["TableRowBgAlt"].as<ImVec4>(ImVec4(1, 1, 1, 0.07f));
      style.Colors[ImGuiCol_TextSelectedBg]       = theme["TextSelectedBg"].as<ImVec4>(ImVec4(0, 0, 1, 0.35f));
      style.Colors[ImGuiCol_DragDropTarget]       = theme["DragDropTarget"].as<ImVec4>(ImVec4(1, 1, 0, 0.9f));
      style.Colors[ImGuiCol_NavHighlight]         = theme["NavHighlight"].as<ImVec4>(ImVec4(0.45f, 0.45f, 0.9f, 0.8f));
      style.Colors[ImGuiCol_NavWindowingHighlight]= theme["NavWindowingHighlight"].as<ImVec4>(ImVec4(1, 1, 1, 0.7f));
      style.Colors[ImGuiCol_NavWindowingDimBg]    = theme["NavWindowingDimBg"].as<ImVec4>(ImVec4(0.8f, 0.8f, 0.8f, 0.2f));
      style.Colors[ImGuiCol_ModalWindowDimBg]     = theme["ModalWindowDimBg"].as<ImVec4>(ImVec4(0.2f, 0.2f, 0.2f, 0.35f));
      

      ResourceID resID = id != 0 ? id : resCounter++;
      m_Resources[resID] = themeRes;
      themeRes->SetResourceID(resID);

      return themeRes;
   }
   template<>
   bool ResourceManager<Ease::EditorTheme>::SaveResource(const char* path, std::shared_ptr<Ease::EditorTheme> resource)
   {
      YAML::Emitter yaml;
      ImGuiStyle& style = resource->style;

      yaml << YAML::BeginMap;
         yaml << YAML::Key << "Type" << YAML::Value << "EditorTheme";
         yaml << YAML::Newline << YAML::Newline;

         yaml << YAML::Key << "Theme";
         yaml << YAML::Value << YAML::BeginMap;
            /* <bool> */
            yaml << YAML::Key << "AntiAliasedLines"       << YAML::Value << style.AntiAliasedLines;
            yaml << YAML::Key << "AntiAliasedLinesUseTex" << YAML::Value << style.AntiAliasedLinesUseTex;
            yaml << YAML::Key << "AntiAliasedFill"        << YAML::Value << style.AntiAliasedFill;
            yaml << YAML::Newline << YAML::Newline;

            /* <float> */
            yaml << YAML::Key << "Alpha"             << YAML::Value << style.Alpha;
            yaml << YAML::Key << "DisabledAlpha"     << YAML::Value << style.DisabledAlpha;
            yaml << YAML::Key << "WindowRounding"    << YAML::Value << style.WindowRounding;
            yaml << YAML::Key << "WindowBorderSize"  << YAML::Value << style.WindowBorderSize;
            yaml << YAML::Key << "ChildRounding"     << YAML::Value << style.ChildRounding;
            yaml << YAML::Key << "ChildBorderSize"   << YAML::Value << style.ChildBorderSize;
            yaml << YAML::Key << "PopupRounding"     << YAML::Value << style.PopupRounding;
            yaml << YAML::Key << "PopupBorderSize"   << YAML::Value << style.PopupBorderSize;
            yaml << YAML::Key << "FrameRounding"     << YAML::Value << style.FrameRounding;
            yaml << YAML::Key << "FrameBorderSize"   << YAML::Value << style.FrameBorderSize;
            yaml << YAML::Key << "IndentSpacing"     << YAML::Value << style.IndentSpacing;
            yaml << YAML::Key << "ColumnsMinSpacing" << YAML::Value << style.ColumnsMinSpacing;
            yaml << YAML::Key << "ScrollbarSize"     << YAML::Value << style.ScrollbarSize;
            yaml << YAML::Key << "ScrollbarRounding" << YAML::Value << style.ScrollbarRounding;
            yaml << YAML::Key << "GrabMinSize"       << YAML::Value << style.GrabMinSize;
            yaml << YAML::Key << "GrabRounding"      << YAML::Value << style.GrabRounding;
            yaml << YAML::Key << "LogSliderDeadzone" << YAML::Value << style.LogSliderDeadzone;
            yaml << YAML::Key << "TabRounding"       << YAML::Value << style.TabRounding;
            yaml << YAML::Key << "TabBorderSize"     << YAML::Value << style.TabBorderSize;
            yaml << YAML::Key << "TabMinWidthForCloseButton"  << YAML::Value << style.TabMinWidthForCloseButton;
            yaml << YAML::Key << "MouseCursorScale"           << YAML::Value << style.MouseCursorScale;
            yaml << YAML::Key << "CurveTessellationTol"       << YAML::Value << style.CurveTessellationTol;
            yaml << YAML::Key << "CircleTessellationMaxError" << YAML::Value << style.CircleTessellationMaxError;
            yaml << YAML::Newline << YAML::Newline;

            /* <ImVec2> */
            yaml << YAML::Key << "WindowPadding"          << YAML::Value << style.WindowPadding;
            yaml << YAML::Key << "WindowMinSize"          << YAML::Value << style.WindowMinSize;
            yaml << YAML::Key << "WindowTitleAlign"       << YAML::Value << style.WindowTitleAlign;
            yaml << YAML::Key << "FramePadding"           << YAML::Value << style.FramePadding;
            yaml << YAML::Key << "ItemSpacing"            << YAML::Value << style.ItemSpacing;
            yaml << YAML::Key << "ItemInnerSpacing"       << YAML::Value << style.ItemInnerSpacing;
            yaml << YAML::Key << "CellPadding"            << YAML::Value << style.CellPadding;
            yaml << YAML::Key << "TouchExtraPadding"      << YAML::Value << style.TouchExtraPadding;
            yaml << YAML::Key << "ButtonTextAlign"        << YAML::Value << style.ButtonTextAlign;
            yaml << YAML::Key << "SelectableTextAlign"    << YAML::Value << style.SelectableTextAlign;
            yaml << YAML::Key << "DisplayWindowPadding"   << YAML::Value << style.DisplayWindowPadding;
            yaml << YAML::Key << "DisplaySafeAreaPadding" << YAML::Value << style.DisplaySafeAreaPadding;
            yaml << YAML::Newline << YAML::Newline;

            /* <ImVec4> */
            yaml << YAML::Key << "Text"                  << YAML::Value << style.Colors[ImGuiCol_Text];
            yaml << YAML::Key << "TextDisabled"          << YAML::Value << style.Colors[ImGuiCol_TextDisabled];
            yaml << YAML::Key << "WindowBg"              << YAML::Value << style.Colors[ImGuiCol_WindowBg];
            yaml << YAML::Key << "ChildBg"               << YAML::Value << style.Colors[ImGuiCol_ChildBg];
            yaml << YAML::Key << "PopupBg"               << YAML::Value << style.Colors[ImGuiCol_PopupBg];
            yaml << YAML::Key << "Border"                << YAML::Value << style.Colors[ImGuiCol_Border];
            yaml << YAML::Key << "BorderShadow"          << YAML::Value << style.Colors[ImGuiCol_BorderShadow];
            yaml << YAML::Key << "FrameBg"               << YAML::Value << style.Colors[ImGuiCol_FrameBg];
            yaml << YAML::Key << "FrameBgHovered"        << YAML::Value << style.Colors[ImGuiCol_FrameBgHovered];
            yaml << YAML::Key << "FrameBgActive"         << YAML::Value << style.Colors[ImGuiCol_FrameBgActive];
            yaml << YAML::Key << "TitleBg"               << YAML::Value << style.Colors[ImGuiCol_TitleBg];
            yaml << YAML::Key << "TitleBgActive"         << YAML::Value << style.Colors[ImGuiCol_TitleBgActive];
            yaml << YAML::Key << "TitleBgCollapsed"      << YAML::Value << style.Colors[ImGuiCol_TitleBgCollapsed];
            yaml << YAML::Key << "MenuBarBg"             << YAML::Value << style.Colors[ImGuiCol_MenuBarBg];
            yaml << YAML::Key << "ScrollbarBg"           << YAML::Value << style.Colors[ImGuiCol_ScrollbarBg];
            yaml << YAML::Key << "ScrollbarGrab"         << YAML::Value << style.Colors[ImGuiCol_ScrollbarGrab];
            yaml << YAML::Key << "ScrollbarGrabHovered"  << YAML::Value << style.Colors[ImGuiCol_ScrollbarGrabHovered];
            yaml << YAML::Key << "ScrollbarGrabActive"   << YAML::Value << style.Colors[ImGuiCol_ScrollbarGrabActive];
            yaml << YAML::Key << "CheckMark"             << YAML::Value << style.Colors[ImGuiCol_CheckMark];
            yaml << YAML::Key << "SliderGrab"            << YAML::Value << style.Colors[ImGuiCol_SliderGrab];
            yaml << YAML::Key << "SliderGrabActive"      << YAML::Value << style.Colors[ImGuiCol_SliderGrabActive];
            yaml << YAML::Key << "Button"                << YAML::Value << style.Colors[ImGuiCol_Button];
            yaml << YAML::Key << "ButtonHovered"         << YAML::Value << style.Colors[ImGuiCol_ButtonHovered];
            yaml << YAML::Key << "ButtonActive"          << YAML::Value << style.Colors[ImGuiCol_ButtonActive];
            yaml << YAML::Key << "Header"                << YAML::Value << style.Colors[ImGuiCol_Header];
            yaml << YAML::Key << "HeaderHovered"         << YAML::Value << style.Colors[ImGuiCol_HeaderHovered];
            yaml << YAML::Key << "HeaderActive"          << YAML::Value << style.Colors[ImGuiCol_HeaderActive];
            yaml << YAML::Key << "Separator"             << YAML::Value << style.Colors[ImGuiCol_Separator];
            yaml << YAML::Key << "SeparatorHovered"      << YAML::Value << style.Colors[ImGuiCol_SeparatorHovered];
            yaml << YAML::Key << "SeparatorActive"       << YAML::Value << style.Colors[ImGuiCol_SeparatorActive];
            yaml << YAML::Key << "ResizeGrip"            << YAML::Value << style.Colors[ImGuiCol_ResizeGrip];
            yaml << YAML::Key << "ResizeGripHovered"     << YAML::Value << style.Colors[ImGuiCol_ResizeGripHovered];
            yaml << YAML::Key << "ResizeGripActive"      << YAML::Value << style.Colors[ImGuiCol_ResizeGripActive];
            yaml << YAML::Key << "Tab"                   << YAML::Value << style.Colors[ImGuiCol_Tab];
            yaml << YAML::Key << "TabHovered"            << YAML::Value << style.Colors[ImGuiCol_TabHovered];
            yaml << YAML::Key << "TabActive"             << YAML::Value << style.Colors[ImGuiCol_TabActive];
            yaml << YAML::Key << "TabUnfocused"          << YAML::Value << style.Colors[ImGuiCol_TabUnfocused];
            yaml << YAML::Key << "TabUnfocusedActive"    << YAML::Value << style.Colors[ImGuiCol_TabUnfocusedActive];
            yaml << YAML::Key << "DockingPreview"        << YAML::Value << style.Colors[ImGuiCol_DockingPreview];
            yaml << YAML::Key << "DockingEmptyBg"        << YAML::Value << style.Colors[ImGuiCol_DockingEmptyBg];
            yaml << YAML::Key << "PlotLines"             << YAML::Value << style.Colors[ImGuiCol_PlotLines];
            yaml << YAML::Key << "PlotLinesHovered"      << YAML::Value << style.Colors[ImGuiCol_PlotLinesHovered];
            yaml << YAML::Key << "PlotHistogram"         << YAML::Value << style.Colors[ImGuiCol_PlotHistogram];
            yaml << YAML::Key << "PlotHistogramHovered"  << YAML::Value << style.Colors[ImGuiCol_PlotHistogramHovered];
            yaml << YAML::Key << "TableHeaderBg"         << YAML::Value << style.Colors[ImGuiCol_TableHeaderBg];
            yaml << YAML::Key << "TableBorderStrong"     << YAML::Value << style.Colors[ImGuiCol_TableBorderStrong];
            yaml << YAML::Key << "TableBorderLight"      << YAML::Value << style.Colors[ImGuiCol_TableBorderLight];
            yaml << YAML::Key << "TableRowBg"            << YAML::Value << style.Colors[ImGuiCol_TableRowBg];
            yaml << YAML::Key << "TableRowBgAlt"         << YAML::Value << style.Colors[ImGuiCol_TableRowBgAlt];
            yaml << YAML::Key << "TextSelectedBg"        << YAML::Value << style.Colors[ImGuiCol_TextSelectedBg];
            yaml << YAML::Key << "DragDropTarget"        << YAML::Value << style.Colors[ImGuiCol_DragDropTarget];
            yaml << YAML::Key << "NavHighlight"          << YAML::Value << style.Colors[ImGuiCol_NavHighlight];
            yaml << YAML::Key << "NavWindowingHighlight" << YAML::Value << style.Colors[ImGuiCol_NavWindowingHighlight];
            yaml << YAML::Key << "NavWindowingDimBg"     << YAML::Value << style.Colors[ImGuiCol_NavWindowingDimBg];
            yaml << YAML::Key << "ModalWindowDimBg"      << YAML::Value << style.Colors[ImGuiCol_ModalWindowDimBg];
            yaml << YAML::Newline;
         yaml << YAML::EndMap;
      yaml << YAML::EndMap;
      
      std::ofstream ofstream(Ease::File::Path(path));
      ofstream << yaml.c_str();
      return true;
   }

   template<>
   ResourceManager<Ease::EditorTheme>& ResourceManager<Ease::EditorTheme>::GetLoader()
   {
      static ResourceManager<Ease::EditorTheme> themeLoader;

      return themeLoader;
   }
   /* ==============</Ease::NativeModule>============== */
   
} // namespace Ease