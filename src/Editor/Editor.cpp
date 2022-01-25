/**
 * @file Editor.cpp
 * @author Lexographics
 * @brief 
 * @version 0.1
 * @date 2022-01-26
 */
#include "Editor.hpp"  

#include "Application/Window.hpp"
#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"
#include <EaseGL.hpp>

#include <Scene/Components.hpp>
#include <Scene/Scene.hpp>
#include <Scene/Entity.hpp>

#include "Resource/Project.hpp"
#include "Application/Application.hpp"

#include <iostream>


Editor::Editor()
{
	const char* glsl_version = "#version 150";
   /* Setup dear imgui */
   IMGUI_CHECKVERSION();
   ImGui::CreateContext();
   ImGuiIO& io = ImGui::GetIO(); (void)io;
   io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
   io.Fonts->AddFontFromFileTTF(Project::get_singleton().GetAbsolutePath("Fonts/Roboto-Medium.ttf").c_str(), 14.f);
   ImGui::StyleColorsDark();
   SetStyle(0);

   //glfwSetCharCallback((GLFWwindow*)Window::get_singleton(), ImGui_ImplGlfw_CharCallback);
   //glfwSetKeyCallback((GLFWwindow*)Window::get_singleton(), ImGui_ImplGlfw_KeyCallback);
   //glfwSetScrollCallback((GLFWwindow*)Window::get_singleton(), ImGui_ImplGlfw_ScrollCallback);

   ImGui_ImplOpenGL3_Init(glsl_version);
   ImGui_ImplGlfw_InitForOpenGL((GLFWwindow*)Window::get_singleton(), true);


   m_PlayTexture.LoadTexture(Project::get_singleton().GetAbsolutePath("Textures/play.png").c_str());
   m_StopTexture.LoadTexture(Project::get_singleton().GetAbsolutePath("Textures/stop.png").c_str());
   
}

// static
Editor& Editor::get_singleton() 
{
   static Editor editor;
   return editor;
}



template<typename T, typename UIFunction>
static void drawComponent(const std::string& name, Entity& entity, UIFunction uiFunc)
{
   if(!entity.hasComponent<T>())
      return;
   auto& component = entity.getComponent<T>();
   
   //ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_SpanFullWidth | ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_NoAutoOpenOnLog;

   ImGui::Separator();
   bool clicked = ImGui::CollapsingHeader(name.c_str(), 0);
   if(clicked)
   {
      ImGui::Indent();
      uiFunc(component);

      if(ImGui::Button( ("Remove " + name).c_str() ))
      {
         entity.removeComponent<T>();
      }

      ImGui::Unindent();
   }
}


void Editor::Update() 
{
   ImGui_ImplOpenGL3_NewFrame();
   ImGui_ImplGlfw_NewFrame();
   ImGui::NewFrame();
   

   static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_PassthruCentralNode;
   ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
   window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
   window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
   
   ImGuiViewport* viewport = ImGui::GetMainViewport();
   ImGui::SetNextWindowPos(viewport->Pos);
   ImGui::SetNextWindowSize(viewport->Size);
   ImGui::SetNextWindowViewport(viewport->ID);

   
   


   if(ImGui::Begin("DockSpace", nullptr, window_flags))
   {
      ImGuiID dockspace_id = ImGui::GetID("DockSpace");
      ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
      
      
      
      if(ImGui::BeginMenuBar())
      {
         if(ImGui::BeginMenu("File"))
         {
            ImGui::EndMenu();
         }
         if(ImGui::BeginMenu("Project"))
         {
            ImGui::EndMenu();
         }
         if(ImGui::BeginMenu("Editor"))
         {
            ImGui::EndMenu();
         }

         ImGui::EndMenuBar();
      }




      if(ImGui::Begin("Viewport"))
      {
         ImTextureID playButtonTexture = (void*)1;
         if(Application::get_singleton().IsRunning())
            playButtonTexture = (void*)(GLuint64)m_StopTexture;
         else
            playButtonTexture = (void*)(GLuint64)m_PlayTexture;

         if(ImGui::ImageButton(ImTextureID(playButtonTexture), ImVec2(24, 24)))
         {
            m_EditorData->start_stop_pressed = true;
         }

         ImGui::SameLine();
         ImGui::Checkbox("In Game Camera", &m_EditorData->useInGameCamera);



         ImGui::Image(ImTextureID((GLuint64)m_EditorData->finalFramebufferTextureID), ImGui::GetContentRegionAvail());


         static ImVec2 mouseDragStart;
         static bool dragging = false;
         if (ImGui::IsMouseClicked(ImGuiMouseButton_Right) && ImGui::IsWindowHovered())
         {
            mouseDragStart = ImGui::GetMousePos();
            dragging = true;
         }
         if(!ImGui::IsMouseDown(ImGuiMouseButton_Right))
         {
            dragging = false;
            m_EditorData->viewportDragMouseDeltaX = 0;
            m_EditorData->viewportDragMouseDeltaY = 0;
         }
         
         
         if(dragging)
         {
            m_EditorData->viewportDragMouseDeltaX = mouseDragStart.x - ImGui::GetMousePos().x;
            m_EditorData->viewportDragMouseDeltaY = mouseDragStart.y - ImGui::GetMousePos().y;
            mouseDragStart = ImGui::GetMousePos();
         }
         

         ImGui::End();
      }
      
      if(ImGui::Begin("Scene"))
      {
         auto view = m_EditorData->currentScene->GetRegistry().view<CommonComponent>();
         for(auto it = view.rbegin(); it != view.rend(); ++it)
         {
            Entity entity(*it, &m_EditorData->currentScene->GetRegistry());

            CommonComponent common = entity.getComponent<CommonComponent>();

            ImGuiSelectableFlags flags = 0;
         

            if(ImGui::Selectable(common.name.c_str(), false, flags))
            //if(ImGui::CollapsingHeader(common.name.c_str(), flags))
            {
               m_EditorData->selectedEntity.SetEntityID(*it);
               m_EditorData->selectedEntity.SetRegistry(&m_EditorData->currentScene->GetRegistry());

               //ImGui::TreePop();
            }
         }

         ImGui::End();
      }
      
      if(ImGui::Begin("Assets"))
      ImGui::End();

      if(ImGui::Begin("Console"))
      {
         if(ImGui::Button("Clear"))
         {
            m_ConsoleText = "";
            EASE_LOG_VERSION();
         }
         ImGui::Text("%s", m_ConsoleText.c_str());
         ImGui::End();
      }

      

      if(ImGui::Begin("Inspector"))
      {
         if(m_EditorData->selectedEntity.IsValid())
         {
            Entity selectedEntity = m_EditorData->selectedEntity;

            auto& commonComponent = selectedEntity.getComponent<CommonComponent>();
            
            
            char nameBuf[512];
            memset(nameBuf, 0, 512);
            strcpy(nameBuf, commonComponent.name.data());
            ImGui::Text("%s", "Name: "); ImGui::SameLine();
            if(ImGui::InputText("##name", nameBuf, 512, ImGuiInputTextFlags_EnterReturnsTrue))
            {
               commonComponent.name = nameBuf;
               if(commonComponent.name.size() == 0)
                  commonComponent.name = "Unnamed";
            }


            // Adding component with right click
            if(ImGui::IsMouseClicked(ImGuiMouseButton_Right) && ImGui::IsWindowHovered())
               ImGui::OpenPopup("INSPECTOR-RCLICK");
            
            if(ImGui::BeginPopup("INSPECTOR-RCLICK"))
            {
               if(ImGui::BeginMenu("Add Component"))
               {
                  if(!selectedEntity.hasComponent<Transform2DComponent>() && ImGui::MenuItem("Transform 2D"))
                  {
                     selectedEntity.addComponent<Transform2DComponent>();
                  }
                  if(!selectedEntity.hasComponent<CameraComponent>() && ImGui::MenuItem("Camera"))
                  {
                     selectedEntity.addComponent<CameraComponent>();
                  }
                  ImGui::EndMenu();
               }

               ImGui::EndPopup();
            }

            // Draw Components              // Maybe set ids like name+id so all components have different ids, not necessary for now
            drawComponent<Transform2DComponent>("Transform 2D", selectedEntity, [](Transform2DComponent& component)
            {
               ImGui::DragFloat2("Pos", &component.position.x);
               ImGui::DragFloat2("Scale", &component.scale.x);

               float deg = glm::degrees(component.rotation);
               ImGui::DragFloat("Rotation", &deg);
               component.rotation = glm::radians(deg);

               if(ImGui::SliderInt("Z Index", &component.zIndex, 0, 1000))
                  Editor::get_singleton().Log << "Z Index Not Implemented" << Log::Endl;
            });
            drawComponent<CameraComponent>("Camera", selectedEntity, [](CameraComponent& component)
            {
               ImGui::Checkbox("Current", &component.current);
            });
            
         }
         ImGui::End();
      }
         
         

      ImGui::End();
   }

   
   ImGui::Render();
   ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}
	
Editor::~Editor()
{
	ImGui_ImplOpenGL3_Shutdown();
   ImGui_ImplGlfw_Shutdown();
   ImGui::DestroyContext();
}


EditorOstream& operator<<(EditorOstream& os, const std::string& str) 
{
	Editor::get_singleton().m_ConsoleText += str;
	return os;
}
EditorOstream& operator<<(EditorOstream& os, Log command) 
{
   if(command == Log::Endl)
	   Editor::get_singleton().m_ConsoleText += "\n";
	return os;
}
EditorOstream& operator<<(EditorOstream& os, int num) 
{
   Editor::get_singleton().m_ConsoleText += std::to_string(num);
	return os;
}




void Editor::SetStyle(int index)
{  
   if(index == 0)
   {
      /**
       * https://github.com/MomoDeve
       */
      constexpr auto ColorFromBytes = [](uint8_t r, uint8_t g, uint8_t b)
      {
         return ImVec4((float)r / 255.0f, (float)g / 255.0f, (float)b / 255.0f, 1.0f);
      };

      auto& style = ImGui::GetStyle();
      ImVec4* colors = style.Colors;

      const ImVec4 bgColor           = ColorFromBytes(37, 37, 38);
      const ImVec4 lightBgColor      = ColorFromBytes(82, 82, 85);
      const ImVec4 veryLightBgColor  = ColorFromBytes(90, 90, 95);

      const ImVec4 panelColor        = ColorFromBytes(51, 51, 55);
      const ImVec4 panelHoverColor   = ColorFromBytes(29, 151, 236);
      const ImVec4 panelActiveColor  = ColorFromBytes(0, 119, 200);

      const ImVec4 textColor         = ColorFromBytes(255, 255, 255);
      const ImVec4 textDisabledColor = ColorFromBytes(151, 151, 151);
      const ImVec4 borderColor       = ColorFromBytes(78, 78, 78);

      colors[ImGuiCol_Text]                 = textColor;
      colors[ImGuiCol_TextDisabled]         = textDisabledColor;
      colors[ImGuiCol_TextSelectedBg]       = panelActiveColor;
      colors[ImGuiCol_WindowBg]             = bgColor;
      colors[ImGuiCol_ChildBg]              = bgColor;
      colors[ImGuiCol_PopupBg]              = bgColor;
      colors[ImGuiCol_Border]               = borderColor;
      colors[ImGuiCol_BorderShadow]         = borderColor;
      colors[ImGuiCol_FrameBg]              = panelColor;
      colors[ImGuiCol_FrameBgHovered]       = panelHoverColor;
      colors[ImGuiCol_FrameBgActive]        = panelActiveColor;
      colors[ImGuiCol_TitleBg]              = bgColor;
      colors[ImGuiCol_TitleBgActive]        = bgColor;
      colors[ImGuiCol_TitleBgCollapsed]     = bgColor;
      colors[ImGuiCol_MenuBarBg]            = panelColor;
      colors[ImGuiCol_ScrollbarBg]          = panelColor;
      colors[ImGuiCol_ScrollbarGrab]        = lightBgColor;
      colors[ImGuiCol_ScrollbarGrabHovered] = veryLightBgColor;
      colors[ImGuiCol_ScrollbarGrabActive]  = veryLightBgColor;
      colors[ImGuiCol_CheckMark]            = panelActiveColor;
      colors[ImGuiCol_SliderGrab]           = panelHoverColor;
      colors[ImGuiCol_SliderGrabActive]     = panelActiveColor;
      colors[ImGuiCol_Button]               = panelColor;
      colors[ImGuiCol_ButtonHovered]        = panelHoverColor;
      colors[ImGuiCol_ButtonActive]         = panelHoverColor;
      colors[ImGuiCol_Header]               = panelColor;
      colors[ImGuiCol_HeaderHovered]        = panelHoverColor;
      colors[ImGuiCol_HeaderActive]         = panelActiveColor;
      colors[ImGuiCol_Separator]            = borderColor;
      colors[ImGuiCol_SeparatorHovered]     = borderColor;
      colors[ImGuiCol_SeparatorActive]      = borderColor;
      colors[ImGuiCol_ResizeGrip]           = bgColor;
      colors[ImGuiCol_ResizeGripHovered]    = panelColor;
      colors[ImGuiCol_ResizeGripActive]     = lightBgColor;
      colors[ImGuiCol_PlotLines]            = panelActiveColor;
      colors[ImGuiCol_PlotLinesHovered]     = panelHoverColor;
      colors[ImGuiCol_PlotHistogram]        = panelActiveColor;
      colors[ImGuiCol_PlotHistogramHovered] = panelHoverColor;
      colors[ImGuiCol_DragDropTarget]       = bgColor;
      colors[ImGuiCol_NavHighlight]         = bgColor;
      colors[ImGuiCol_DockingPreview]       = panelActiveColor;
      colors[ImGuiCol_Tab]                  = bgColor;
      colors[ImGuiCol_TabActive]            = panelActiveColor;
      colors[ImGuiCol_TabUnfocused]         = bgColor;
      colors[ImGuiCol_TabUnfocusedActive]   = panelActiveColor;
      colors[ImGuiCol_TabHovered]           = panelHoverColor;

      style.WindowRounding    = 0.0f;
      style.ChildRounding     = 0.0f;
      style.FrameRounding     = 0.0f;
      style.GrabRounding      = 0.0f;
      style.PopupRounding     = 0.0f;
      style.ScrollbarRounding = 0.0f;
      style.TabRounding       = 0.0f;
   }
   if(index == 1)
   {
      constexpr auto ColorFromBytes = [](uint8_t r, uint8_t g, uint8_t b)
      {
         return ImVec4((float)r / 500.0f, (float)g / 400.0f, (float)b / 400.0f, 1.0f);
      };

      auto& style = ImGui::GetStyle();
      ImVec4* colors = style.Colors;

      const ImVec4 bgColor           = ColorFromBytes(37, 37, 38);
      const ImVec4 lightBgColor      = ColorFromBytes(82, 82, 85);
      const ImVec4 veryLightBgColor  = ColorFromBytes(90, 90, 95);

      const ImVec4 panelColor        = ColorFromBytes(51, 51, 55);
      const ImVec4 panelHoverColor   = ColorFromBytes(29, 151, 236);
      const ImVec4 panelActiveColor  = ColorFromBytes(0, 119, 200);

      const ImVec4 textColor         = ColorFromBytes(255, 255, 255);
      const ImVec4 textDisabledColor = ColorFromBytes(151, 151, 151);
      const ImVec4 borderColor       = ColorFromBytes(78, 78, 78);

      colors[ImGuiCol_Text]                 = textColor;
      colors[ImGuiCol_TextDisabled]         = textDisabledColor;
      colors[ImGuiCol_TextSelectedBg]       = panelActiveColor;
      colors[ImGuiCol_WindowBg]             = bgColor;
      colors[ImGuiCol_ChildBg]              = bgColor;
      colors[ImGuiCol_PopupBg]              = bgColor;
      colors[ImGuiCol_Border]               = borderColor;
      colors[ImGuiCol_BorderShadow]         = borderColor;
      colors[ImGuiCol_FrameBg]              = panelColor;
      colors[ImGuiCol_FrameBgHovered]       = panelHoverColor;
      colors[ImGuiCol_FrameBgActive]        = panelActiveColor;
      colors[ImGuiCol_TitleBg]              = bgColor;
      colors[ImGuiCol_TitleBgActive]        = bgColor;
      colors[ImGuiCol_TitleBgCollapsed]     = bgColor;
      colors[ImGuiCol_MenuBarBg]            = panelColor;
      colors[ImGuiCol_ScrollbarBg]          = panelColor;
      colors[ImGuiCol_ScrollbarGrab]        = lightBgColor;
      colors[ImGuiCol_ScrollbarGrabHovered] = veryLightBgColor;
      colors[ImGuiCol_ScrollbarGrabActive]  = veryLightBgColor;
      colors[ImGuiCol_CheckMark]            = panelActiveColor;
      colors[ImGuiCol_SliderGrab]           = panelHoverColor;
      colors[ImGuiCol_SliderGrabActive]     = panelActiveColor;
      colors[ImGuiCol_Button]               = panelColor;
      colors[ImGuiCol_ButtonHovered]        = panelHoverColor;
      colors[ImGuiCol_ButtonActive]         = panelHoverColor;
      colors[ImGuiCol_Header]               = panelColor;
      colors[ImGuiCol_HeaderHovered]        = panelHoverColor;
      colors[ImGuiCol_HeaderActive]         = panelActiveColor;
      colors[ImGuiCol_Separator]            = borderColor;
      colors[ImGuiCol_SeparatorHovered]     = borderColor;
      colors[ImGuiCol_SeparatorActive]      = borderColor;
      colors[ImGuiCol_ResizeGrip]           = bgColor;
      colors[ImGuiCol_ResizeGripHovered]    = panelColor;
      colors[ImGuiCol_ResizeGripActive]     = lightBgColor;
      colors[ImGuiCol_PlotLines]            = panelActiveColor;
      colors[ImGuiCol_PlotLinesHovered]     = panelHoverColor;
      colors[ImGuiCol_PlotHistogram]        = panelActiveColor;
      colors[ImGuiCol_PlotHistogramHovered] = panelHoverColor;
      colors[ImGuiCol_DragDropTarget]       = bgColor;
      colors[ImGuiCol_NavHighlight]         = bgColor;
      colors[ImGuiCol_DockingPreview]       = panelActiveColor;
      colors[ImGuiCol_Tab]                  = bgColor;
      colors[ImGuiCol_TabActive]            = panelActiveColor;
      colors[ImGuiCol_TabUnfocused]         = bgColor;
      colors[ImGuiCol_TabUnfocusedActive]   = panelActiveColor;
      colors[ImGuiCol_TabHovered]           = panelHoverColor;

      style.WindowRounding    = 0.0f;
      style.ChildRounding     = 0.0f;
      style.FrameRounding     = 0.0f;
      style.GrabRounding      = 0.0f;
      style.PopupRounding     = 0.0f;
      style.ScrollbarRounding = 0.0f;
      style.TabRounding       = 0.0f;
   }
   else if(index == 2)
   {
      /**
       * https://github.com/janekb04
       */
      ImVec4* colors = ImGui::GetStyle().Colors;
      colors[ImGuiCol_Text]                   = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
      colors[ImGuiCol_TextDisabled]           = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
      colors[ImGuiCol_WindowBg]               = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
      colors[ImGuiCol_ChildBg]                = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
      colors[ImGuiCol_PopupBg]                = ImVec4(0.19f, 0.19f, 0.19f, 0.92f);
      colors[ImGuiCol_Border]                 = ImVec4(0.19f, 0.19f, 0.19f, 0.29f);
      colors[ImGuiCol_BorderShadow]           = ImVec4(0.00f, 0.00f, 0.00f, 0.24f);
      colors[ImGuiCol_FrameBg]                = ImVec4(0.05f, 0.05f, 0.05f, 0.54f);
      colors[ImGuiCol_FrameBgHovered]         = ImVec4(0.19f, 0.19f, 0.19f, 0.54f);
      colors[ImGuiCol_FrameBgActive]          = ImVec4(0.20f, 0.22f, 0.23f, 1.00f);
      colors[ImGuiCol_TitleBg]                = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
      colors[ImGuiCol_TitleBgActive]          = ImVec4(0.06f, 0.06f, 0.06f, 1.00f);
      colors[ImGuiCol_TitleBgCollapsed]       = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
      colors[ImGuiCol_MenuBarBg]              = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
      colors[ImGuiCol_ScrollbarBg]            = ImVec4(0.05f, 0.05f, 0.05f, 0.54f);
      colors[ImGuiCol_ScrollbarGrab]          = ImVec4(0.34f, 0.34f, 0.34f, 0.54f);
      colors[ImGuiCol_ScrollbarGrabHovered]   = ImVec4(0.40f, 0.40f, 0.40f, 0.54f);
      colors[ImGuiCol_ScrollbarGrabActive]    = ImVec4(0.56f, 0.56f, 0.56f, 0.54f);
      colors[ImGuiCol_CheckMark]              = ImVec4(0.33f, 0.67f, 0.86f, 1.00f);
      colors[ImGuiCol_SliderGrab]             = ImVec4(0.34f, 0.34f, 0.34f, 0.54f);
      colors[ImGuiCol_SliderGrabActive]       = ImVec4(0.56f, 0.56f, 0.56f, 0.54f);
      colors[ImGuiCol_Button]                 = ImVec4(0.05f, 0.05f, 0.05f, 0.54f);
      colors[ImGuiCol_ButtonHovered]          = ImVec4(0.19f, 0.19f, 0.19f, 0.54f);
      colors[ImGuiCol_ButtonActive]           = ImVec4(0.20f, 0.22f, 0.23f, 1.00f);
      colors[ImGuiCol_Header]                 = ImVec4(0.00f, 0.00f, 0.00f, 0.52f);
      colors[ImGuiCol_HeaderHovered]          = ImVec4(0.00f, 0.00f, 0.00f, 0.36f);
      colors[ImGuiCol_HeaderActive]           = ImVec4(0.20f, 0.22f, 0.23f, 0.33f);
      colors[ImGuiCol_Separator]              = ImVec4(0.28f, 0.28f, 0.28f, 0.29f);
      colors[ImGuiCol_SeparatorHovered]       = ImVec4(0.44f, 0.44f, 0.44f, 0.29f);
      colors[ImGuiCol_SeparatorActive]        = ImVec4(0.40f, 0.44f, 0.47f, 1.00f);
      colors[ImGuiCol_ResizeGrip]             = ImVec4(0.28f, 0.28f, 0.28f, 0.29f);
      colors[ImGuiCol_ResizeGripHovered]      = ImVec4(0.44f, 0.44f, 0.44f, 0.29f);
      colors[ImGuiCol_ResizeGripActive]       = ImVec4(0.40f, 0.44f, 0.47f, 1.00f);
      colors[ImGuiCol_Tab]                    = ImVec4(0.00f, 0.00f, 0.00f, 0.52f);
      colors[ImGuiCol_TabHovered]             = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
      colors[ImGuiCol_TabActive]              = ImVec4(0.20f, 0.20f, 0.20f, 0.36f);
      colors[ImGuiCol_TabUnfocused]           = ImVec4(0.00f, 0.00f, 0.00f, 0.52f);
      colors[ImGuiCol_TabUnfocusedActive]     = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
      colors[ImGuiCol_DockingPreview]         = ImVec4(0.33f, 0.67f, 0.86f, 1.00f);
      colors[ImGuiCol_DockingEmptyBg]         = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
      colors[ImGuiCol_PlotLines]              = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
      colors[ImGuiCol_PlotLinesHovered]       = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
      colors[ImGuiCol_PlotHistogram]          = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
      colors[ImGuiCol_PlotHistogramHovered]   = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
      colors[ImGuiCol_TableHeaderBg]          = ImVec4(0.00f, 0.00f, 0.00f, 0.52f);
      colors[ImGuiCol_TableBorderStrong]      = ImVec4(0.00f, 0.00f, 0.00f, 0.52f);
      colors[ImGuiCol_TableBorderLight]       = ImVec4(0.28f, 0.28f, 0.28f, 0.29f);
      colors[ImGuiCol_TableRowBg]             = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
      colors[ImGuiCol_TableRowBgAlt]          = ImVec4(1.00f, 1.00f, 1.00f, 0.06f);
      colors[ImGuiCol_TextSelectedBg]         = ImVec4(0.20f, 0.22f, 0.23f, 1.00f);
      colors[ImGuiCol_DragDropTarget]         = ImVec4(0.33f, 0.67f, 0.86f, 1.00f);
      colors[ImGuiCol_NavHighlight]           = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
      colors[ImGuiCol_NavWindowingHighlight]  = ImVec4(1.00f, 0.00f, 0.00f, 0.70f);
      colors[ImGuiCol_NavWindowingDimBg]      = ImVec4(1.00f, 0.00f, 0.00f, 0.20f);
      colors[ImGuiCol_ModalWindowDimBg]       = ImVec4(1.00f, 0.00f, 0.00f, 0.35f);

      ImGuiStyle& style = ImGui::GetStyle();
      style.WindowPadding                     = ImVec2(8.00f, 8.00f);
      style.FramePadding                      = ImVec2(5.00f, 2.00f);
      style.CellPadding                       = ImVec2(6.00f, 6.00f);
      style.ItemSpacing                       = ImVec2(6.00f, 6.00f);
      style.ItemInnerSpacing                  = ImVec2(6.00f, 6.00f);
      style.TouchExtraPadding                 = ImVec2(0.00f, 0.00f);
      style.IndentSpacing                     = 25;
      style.ScrollbarSize                     = 15;
      style.GrabMinSize                       = 10;
      style.WindowBorderSize                  = 1;
      style.ChildBorderSize                   = 1;
      style.PopupBorderSize                   = 1;
      style.FrameBorderSize                   = 1;
      style.TabBorderSize                     = 1;
      style.WindowRounding                    = 7;
      style.ChildRounding                     = 4;
      style.FrameRounding                     = 3;
      style.PopupRounding                     = 4;
      style.ScrollbarRounding                 = 9;
      style.GrabRounding                      = 3;
      style.LogSliderDeadzone                 = 4;
      style.TabRounding                       = 4;
   }

}

