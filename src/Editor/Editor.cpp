#include "Editor.hpp"

#include "Ease.hpp"

#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"
#include "ImGuizmo/ImGuizmo.h"

#include "Renderer/Texture/Texture.hpp"



// sets style with given index
// 0 -
static void setStyle(uint32_t index);

enum class EditorMenuBarSelection
{

};

namespace Ease
{

static Texture tex_PlayButton;
static Texture tex_StopButton;
static Texture tex_Directory;
static Texture tex_File;



void Editor::init()
{
   // initialize imgui
   const char* glsl_version =  "#version 150";
   
   /* Setup dear imgui */
   IMGUI_CHECKVERSION();
   ImGui::CreateContext();
   ImGuiIO& io = ImGui::GetIO(); (void)io;
   io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
   ImGui::StyleColorsDark();
   io.Fonts->AddFontFromFileTTF("res/fonts/Roboto-Medium.ttf", 16.f);
   setStyle(4);

   glfwSetCharCallback(editorData.window->get(), ImGui_ImplGlfw_CharCallback);
   glfwSetKeyCallback(editorData.window->get(), ImGui_ImplGlfw_KeyCallback);
   glfwSetScrollCallback(editorData.window->get(), ImGui_ImplGlfw_ScrollCallback);

   if(io.WantCaptureKeyboard)
      std::cout << "Keyboard" << std::endl;
   if(io.WantCaptureMouse)
      std::cout << "Mouse" << std::endl;
   

   ImGui_ImplGlfw_InitForOpenGL(editorData.window->get(), false);
   ImGui_ImplOpenGL3_Init(glsl_version);
   

   
   tex_PlayButton.setFilePath("res://textures/editor/play.png");
   tex_PlayButton.recreateTexture();
   
   tex_StopButton.setFilePath("res://textures/editor/stop.png");
   tex_StopButton.recreateTexture();
   
   tex_Directory.setFilePath("res://textures/editor/directory.png");
   tex_Directory.recreateTexture();
   
   tex_File.setFilePath("res://textures/editor/file.png");
   tex_File.recreateTexture();

   editorData.tex_PlayButton = &tex_PlayButton;
   editorData.tex_StopButton = &tex_StopButton;
   editorData.tex_Directory  = &tex_Directory;
   editorData.tex_File       = &tex_File;
}

void Editor::beginDraw()
{
   ImGui_ImplOpenGL3_NewFrame();
   ImGui_ImplGlfw_NewFrame();
   ImGui::NewFrame();
   ImGuizmo::BeginFrame();
}

EditorReturnData Editor::update()
{
   m_EditorReturnData = EditorReturnData();

   static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_PassthruCentralNode;
   ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
   window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
   window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
   //if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode)
      //window_flags |= ImGuiWindowFlags_NoBackground;

   ImGuiViewport* viewport = ImGui::GetMainViewport();
   ImGui::SetNextWindowPos(viewport->Pos);
   ImGui::SetNextWindowSize(viewport->Size);
   ImGui::SetNextWindowViewport(viewport->ID);
   ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
   ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

   

   ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
   ImGui::Begin("DockSpace", nullptr, window_flags);
      ImGui::PopStyleVar();
      ImGui::PopStyleVar(2);

      ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
      ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
      
      ImGui::ShowDemoWindow();

      
      drawMenuBar();
      drawViewport();
      drawContentBrowser();
      drawInspector();
      drawSceneHierarchy();
      drawConsole();
      


   ImGui::End();


   return m_EditorReturnData;
}

void Editor::endDraw()
{
   ImGui::Render();
   int display_w, display_h;
   glfwGetFramebufferSize(editorData.window->get(), &display_w, &display_h);
   ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

Editor::~Editor()
{
   ImGui_ImplOpenGL3_Shutdown();
   ImGui_ImplGlfw_Shutdown();
   ImGui::DestroyContext();
}

} // namespace Ease




void setStyle(uint32_t index)
{
   if(index == 0)
   {
      ImGuiStyle * style = &ImGui::GetStyle();

      style->WindowPadding = ImVec2(15, 15);
      style->WindowRounding = 5.0f;
      style->FramePadding = ImVec2(5, 5);
      style->FrameRounding = 4.0f;
      style->ItemSpacing = ImVec2(12, 8);
      style->ItemInnerSpacing = ImVec2(8, 6);
      style->IndentSpacing = 25.0f;
      style->ScrollbarSize = 15.0f;
      style->ScrollbarRounding = 9.0f;
      style->GrabMinSize = 5.0f;
      style->GrabRounding = 3.0f;
      style->FrameBorderSize = 1.0f;

      style->Colors[ImGuiCol_Text] = ImVec4(0.80f, 0.80f, 0.83f, 1.00f);
      style->Colors[ImGuiCol_TextDisabled] = ImVec4(0.24f, 0.23f, 0.29f, 1.00f);
      style->Colors[ImGuiCol_WindowBg] = ImVec4(0.06f, 0.05f, 0.07f, 1.00f);
      style->Colors[ImGuiCol_PopupBg] = ImVec4(0.07f, 0.07f, 0.09f, 1.00f);
      style->Colors[ImGuiCol_Border] = ImVec4(0.80f, 0.80f, 0.83f, 0.88f);
      style->Colors[ImGuiCol_BorderShadow] = ImVec4(0.92f, 0.91f, 0.88f, 0.00f);
      style->Colors[ImGuiCol_FrameBg] = ImVec4(0.10f, 0.09f, 0.12f, 1.00f);
      style->Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.24f, 0.23f, 0.29f, 1.00f);
      style->Colors[ImGuiCol_FrameBgActive] = ImVec4(0.56f, 0.56f, 0.58f, 1.00f);
      style->Colors[ImGuiCol_TitleBg] = ImVec4(0.10f, 0.09f, 0.12f, 1.00f);
      style->Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(1.00f, 0.98f, 0.95f, 0.75f);
      style->Colors[ImGuiCol_TitleBgActive] = ImVec4(0.07f, 0.07f, 0.09f, 1.00f);
      style->Colors[ImGuiCol_MenuBarBg] = ImVec4(0.10f, 0.09f, 0.12f, 1.00f);
      style->Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.10f, 0.09f, 0.12f, 1.00f);
      style->Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.80f, 0.80f, 0.83f, 0.31f);
      style->Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.56f, 0.56f, 0.58f, 1.00f);
      style->Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.06f, 0.05f, 0.07f, 1.00f);
      style->Colors[ImGuiCol_CheckMark] = ImVec4(0.80f, 0.80f, 0.83f, 0.31f);
      style->Colors[ImGuiCol_SliderGrab] = ImVec4(0.80f, 0.80f, 0.83f, 0.31f);
      style->Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.06f, 0.05f, 0.07f, 1.00f);
      style->Colors[ImGuiCol_Button] = ImVec4(0.10f, 0.09f, 0.12f, 1.00f);
      style->Colors[ImGuiCol_ButtonHovered] = ImVec4(0.24f, 0.23f, 0.29f, 1.00f);
      style->Colors[ImGuiCol_ButtonActive] = ImVec4(0.56f, 0.56f, 0.58f, 1.00f);
      style->Colors[ImGuiCol_Header] = ImVec4(0.10f, 0.09f, 0.12f, 1.00f);
      style->Colors[ImGuiCol_HeaderHovered] = ImVec4(0.56f, 0.56f, 0.58f, 1.00f);
      style->Colors[ImGuiCol_HeaderActive] = ImVec4(0.06f, 0.05f, 0.07f, 1.00f);
      style->Colors[ImGuiCol_ResizeGrip] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
      style->Colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.56f, 0.56f, 0.58f, 1.00f);
      style->Colors[ImGuiCol_ResizeGripActive] = ImVec4(0.06f, 0.05f, 0.07f, 1.00f);
      style->Colors[ImGuiCol_PlotLines] = ImVec4(0.40f, 0.39f, 0.38f, 0.63f);
      style->Colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.25f, 1.00f, 0.00f, 1.00f);
      style->Colors[ImGuiCol_PlotHistogram] = ImVec4(0.40f, 0.39f, 0.38f, 0.63f);
      style->Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.25f, 1.00f, 0.00f, 1.00f);
      style->Colors[ImGuiCol_TextSelectedBg] = ImVec4(0.25f, 1.00f, 0.00f, 0.43f);
   }
   else if(index == 1)
   {
      ImGuiStyle* style = &ImGui::GetStyle();
      ImVec4* colors = style->Colors;

      colors[ImGuiCol_Text]                   = ImVec4(1.000f, 1.000f, 1.000f, 1.000f);
      colors[ImGuiCol_TextDisabled]           = ImVec4(0.500f, 0.500f, 0.500f, 1.000f);
      colors[ImGuiCol_WindowBg]               = ImVec4(0.180f, 0.180f, 0.180f, 1.000f);
      colors[ImGuiCol_ChildBg]                = ImVec4(0.280f, 0.280f, 0.280f, 0.000f);
      colors[ImGuiCol_PopupBg]                = ImVec4(0.313f, 0.313f, 0.313f, 1.000f);
      colors[ImGuiCol_Border]                 = ImVec4(0.266f, 0.266f, 0.266f, 1.000f);
      colors[ImGuiCol_BorderShadow]           = ImVec4(0.000f, 0.000f, 0.000f, 0.000f);
      colors[ImGuiCol_FrameBg]                = ImVec4(0.160f, 0.160f, 0.160f, 1.000f);
      colors[ImGuiCol_FrameBgHovered]         = ImVec4(0.200f, 0.200f, 0.200f, 1.000f);
      colors[ImGuiCol_FrameBgActive]          = ImVec4(0.280f, 0.280f, 0.280f, 1.000f);
      colors[ImGuiCol_TitleBg]                = ImVec4(0.148f, 0.148f, 0.148f, 1.000f);
      colors[ImGuiCol_TitleBgActive]          = ImVec4(0.148f, 0.148f, 0.148f, 1.000f);
      colors[ImGuiCol_TitleBgCollapsed]       = ImVec4(0.148f, 0.148f, 0.148f, 1.000f);
      colors[ImGuiCol_MenuBarBg]              = ImVec4(0.195f, 0.195f, 0.195f, 1.000f);
      colors[ImGuiCol_ScrollbarBg]            = ImVec4(0.160f, 0.160f, 0.160f, 1.000f);
      colors[ImGuiCol_ScrollbarGrab]          = ImVec4(0.277f, 0.277f, 0.277f, 1.000f);
      colors[ImGuiCol_ScrollbarGrabHovered]   = ImVec4(0.300f, 0.300f, 0.300f, 1.000f);
      colors[ImGuiCol_ScrollbarGrabActive]    = ImVec4(1.000f, 0.391f, 0.000f, 1.000f);
      colors[ImGuiCol_CheckMark]              = ImVec4(1.000f, 1.000f, 1.000f, 1.000f);
      colors[ImGuiCol_SliderGrab]             = ImVec4(0.391f, 0.391f, 0.391f, 1.000f);
      colors[ImGuiCol_SliderGrabActive]       = ImVec4(1.000f, 0.391f, 0.000f, 1.000f);
      colors[ImGuiCol_Button]                 = ImVec4(1.000f, 1.000f, 1.000f, 0.000f);
      colors[ImGuiCol_ButtonHovered]          = ImVec4(1.000f, 1.000f, 1.000f, 0.156f);
      colors[ImGuiCol_ButtonActive]           = ImVec4(1.000f, 1.000f, 1.000f, 0.391f);
      colors[ImGuiCol_Header]                 = ImVec4(0.313f, 0.313f, 0.313f, 1.000f);
      colors[ImGuiCol_HeaderHovered]          = ImVec4(0.469f, 0.469f, 0.469f, 1.000f);
      colors[ImGuiCol_HeaderActive]           = ImVec4(0.469f, 0.469f, 0.469f, 1.000f);
      colors[ImGuiCol_Separator]              = colors[ImGuiCol_Border];
      colors[ImGuiCol_SeparatorHovered]       = ImVec4(0.391f, 0.391f, 0.391f, 1.000f);
      colors[ImGuiCol_SeparatorActive]        = ImVec4(1.000f, 0.391f, 0.000f, 1.000f);
      colors[ImGuiCol_ResizeGrip]             = ImVec4(1.000f, 1.000f, 1.000f, 0.250f);
      colors[ImGuiCol_ResizeGripHovered]      = ImVec4(1.000f, 1.000f, 1.000f, 0.670f);
      colors[ImGuiCol_ResizeGripActive]       = ImVec4(1.000f, 0.391f, 0.000f, 1.000f);
      colors[ImGuiCol_Tab]                    = ImVec4(0.098f, 0.098f, 0.098f, 1.000f);
      colors[ImGuiCol_TabHovered]             = ImVec4(0.352f, 0.352f, 0.352f, 1.000f);
      colors[ImGuiCol_TabActive]              = ImVec4(0.195f, 0.195f, 0.195f, 1.000f);
      colors[ImGuiCol_TabUnfocused]           = ImVec4(0.098f, 0.098f, 0.098f, 1.000f);
      colors[ImGuiCol_TabUnfocusedActive]     = ImVec4(0.195f, 0.195f, 0.195f, 1.000f);
      colors[ImGuiCol_DockingPreview]         = ImVec4(1.000f, 0.391f, 0.000f, 0.781f);
      colors[ImGuiCol_DockingEmptyBg]         = ImVec4(0.180f, 0.180f, 0.180f, 1.000f);
      colors[ImGuiCol_PlotLines]              = ImVec4(0.469f, 0.469f, 0.469f, 1.000f);
      colors[ImGuiCol_PlotLinesHovered]       = ImVec4(1.000f, 0.391f, 0.000f, 1.000f);
      colors[ImGuiCol_PlotHistogram]          = ImVec4(0.586f, 0.586f, 0.586f, 1.000f);
      colors[ImGuiCol_PlotHistogramHovered]   = ImVec4(1.000f, 0.391f, 0.000f, 1.000f);
      colors[ImGuiCol_TextSelectedBg]         = ImVec4(1.000f, 1.000f, 1.000f, 0.156f);
      colors[ImGuiCol_DragDropTarget]         = ImVec4(1.000f, 0.391f, 0.000f, 1.000f);
      colors[ImGuiCol_NavHighlight]           = ImVec4(1.000f, 0.391f, 0.000f, 1.000f);
      colors[ImGuiCol_NavWindowingHighlight]  = ImVec4(1.000f, 0.391f, 0.000f, 1.000f);
      colors[ImGuiCol_NavWindowingDimBg]      = ImVec4(0.000f, 0.000f, 0.000f, 0.586f);
      colors[ImGuiCol_ModalWindowDimBg]       = ImVec4(0.000f, 0.000f, 0.000f, 0.586f);

      style->ChildRounding = 4.0f;
      style->FrameBorderSize = 1.0f;
      style->FrameRounding = 2.0f;
      style->GrabMinSize = 7.0f;
      style->PopupRounding = 2.0f;
      style->ScrollbarRounding = 12.0f;
      style->ScrollbarSize = 13.0f;
      style->TabBorderSize = 1.0f;
      style->TabRounding = 0.0f;
      style->WindowRounding = 4.0f;
   }
   else if(index == 2)
   {
      ImGuiStyle & style = ImGui::GetStyle();
      ImVec4 * colors = style.Colors;
      
      /// 0 = FLAT APPEARENCE
      /// 1 = MORE "3D" LOOK
      int is3D = 1;
         
      colors[ImGuiCol_Text]                   = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
      colors[ImGuiCol_TextDisabled]           = ImVec4(0.40f, 0.40f, 0.40f, 1.00f);
      colors[ImGuiCol_ChildBg]                = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
      colors[ImGuiCol_WindowBg]               = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
      colors[ImGuiCol_PopupBg]                = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
      colors[ImGuiCol_Border]                 = ImVec4(0.12f, 0.12f, 0.12f, 0.71f);
      colors[ImGuiCol_BorderShadow]           = ImVec4(1.00f, 1.00f, 1.00f, 0.06f);
      colors[ImGuiCol_FrameBg]                = ImVec4(0.42f, 0.42f, 0.42f, 0.54f);
      colors[ImGuiCol_FrameBgHovered]         = ImVec4(0.42f, 0.42f, 0.42f, 0.40f);
      colors[ImGuiCol_FrameBgActive]          = ImVec4(0.56f, 0.56f, 0.56f, 0.67f);
      colors[ImGuiCol_TitleBg]                = ImVec4(0.19f, 0.19f, 0.19f, 1.00f);
      colors[ImGuiCol_TitleBgActive]          = ImVec4(0.22f, 0.22f, 0.22f, 1.00f);
      colors[ImGuiCol_TitleBgCollapsed]       = ImVec4(0.17f, 0.17f, 0.17f, 0.90f);
      colors[ImGuiCol_MenuBarBg]              = ImVec4(0.335f, 0.335f, 0.335f, 1.000f);
      colors[ImGuiCol_ScrollbarBg]            = ImVec4(0.24f, 0.24f, 0.24f, 0.53f);
      colors[ImGuiCol_ScrollbarGrab]          = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
      colors[ImGuiCol_ScrollbarGrabHovered]   = ImVec4(0.52f, 0.52f, 0.52f, 1.00f);
      colors[ImGuiCol_ScrollbarGrabActive]    = ImVec4(0.76f, 0.76f, 0.76f, 1.00f);
      colors[ImGuiCol_CheckMark]              = ImVec4(0.65f, 0.65f, 0.65f, 1.00f);
      colors[ImGuiCol_SliderGrab]             = ImVec4(0.52f, 0.52f, 0.52f, 1.00f);
      colors[ImGuiCol_SliderGrabActive]       = ImVec4(0.64f, 0.64f, 0.64f, 1.00f);
      colors[ImGuiCol_Button]                 = ImVec4(0.54f, 0.54f, 0.54f, 0.35f);
      colors[ImGuiCol_ButtonHovered]          = ImVec4(0.52f, 0.52f, 0.52f, 0.59f);
      colors[ImGuiCol_ButtonActive]           = ImVec4(0.76f, 0.76f, 0.76f, 1.00f);
      colors[ImGuiCol_Header]                 = ImVec4(0.38f, 0.38f, 0.38f, 1.00f);
      colors[ImGuiCol_HeaderHovered]          = ImVec4(0.47f, 0.47f, 0.47f, 1.00f);
      colors[ImGuiCol_HeaderActive]           = ImVec4(0.76f, 0.76f, 0.76f, 0.77f);
      colors[ImGuiCol_Separator]              = ImVec4(0.000f, 0.000f, 0.000f, 0.137f);
      colors[ImGuiCol_SeparatorHovered]       = ImVec4(0.700f, 0.671f, 0.600f, 0.290f);
      colors[ImGuiCol_SeparatorActive]        = ImVec4(0.702f, 0.671f, 0.600f, 0.674f);
      colors[ImGuiCol_ResizeGrip]             = ImVec4(0.26f, 0.59f, 0.98f, 0.25f);
      colors[ImGuiCol_ResizeGripHovered]      = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
      colors[ImGuiCol_ResizeGripActive]       = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
      colors[ImGuiCol_PlotLines]              = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
      colors[ImGuiCol_PlotLinesHovered]       = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
      colors[ImGuiCol_PlotHistogram]          = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
      colors[ImGuiCol_PlotHistogramHovered]   = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
      colors[ImGuiCol_TextSelectedBg]         = ImVec4(0.73f, 0.73f, 0.73f, 0.35f);
      colors[ImGuiCol_ModalWindowDimBg]       = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);
      colors[ImGuiCol_DragDropTarget]         = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
      colors[ImGuiCol_NavHighlight]           = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
      colors[ImGuiCol_NavWindowingHighlight]  = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
      colors[ImGuiCol_NavWindowingDimBg]      = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);

      style.PopupRounding = 3;

      style.WindowPadding = ImVec2(4, 4);
      style.FramePadding  = ImVec2(6, 4);
      style.ItemSpacing   = ImVec2(6, 2);

      style.ScrollbarSize = 18;

      style.WindowBorderSize = 1;
      style.ChildBorderSize  = 1;
      style.PopupBorderSize  = 1;
      style.FrameBorderSize  = is3D; 

      style.WindowRounding    = 3;
      style.ChildRounding     = 3;
      style.FrameRounding     = 3;
      style.ScrollbarRounding = 2;
      style.GrabRounding      = 3;

      #ifdef IMGUI_HAS_DOCK 
         style.TabBorderSize = is3D; 
         style.TabRounding   = 3;

         colors[ImGuiCol_DockingEmptyBg]     = ImVec4(0.38f, 0.38f, 0.38f, 1.00f);
         colors[ImGuiCol_Tab]                = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
         colors[ImGuiCol_TabHovered]         = ImVec4(0.40f, 0.40f, 0.40f, 1.00f);
         colors[ImGuiCol_TabActive]          = ImVec4(0.33f, 0.33f, 0.33f, 1.00f);
         colors[ImGuiCol_TabUnfocused]       = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
         colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.33f, 0.33f, 0.33f, 1.00f);
         colors[ImGuiCol_DockingPreview]     = ImVec4(0.85f, 0.85f, 0.85f, 0.28f);

         if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
         {
            style.WindowRounding = 0.0f;
            style.Colors[ImGuiCol_WindowBg].w = 1.0f;
         }
      #endif
   }
   else if(index == 3)
   {
      ImGui::GetStyle().FrameRounding = 4.0f;
      ImGui::GetStyle().GrabRounding = 4.0f;
      
      ImVec4* colors = ImGui::GetStyle().Colors;
      colors[ImGuiCol_Text] = ImVec4(0.95f, 0.96f, 0.98f, 1.00f);
      colors[ImGuiCol_TextDisabled] = ImVec4(0.36f, 0.42f, 0.47f, 1.00f);
      colors[ImGuiCol_WindowBg] = ImVec4(0.11f, 0.15f, 0.17f, 1.00f);
      colors[ImGuiCol_ChildBg] = ImVec4(0.15f, 0.18f, 0.22f, 1.00f);
      colors[ImGuiCol_PopupBg] = ImVec4(0.08f, 0.08f, 0.08f, 0.94f);
      colors[ImGuiCol_Border] = ImVec4(0.08f, 0.10f, 0.12f, 1.00f);
      colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
      colors[ImGuiCol_FrameBg] = ImVec4(0.20f, 0.25f, 0.29f, 1.00f);
      colors[ImGuiCol_FrameBgHovered] = ImVec4(0.12f, 0.20f, 0.28f, 1.00f);
      colors[ImGuiCol_FrameBgActive] = ImVec4(0.09f, 0.12f, 0.14f, 1.00f);
      colors[ImGuiCol_TitleBg] = ImVec4(0.09f, 0.12f, 0.14f, 0.65f);
      colors[ImGuiCol_TitleBgActive] = ImVec4(0.08f, 0.10f, 0.12f, 1.00f);
      colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.00f, 0.00f, 0.00f, 0.51f);
      colors[ImGuiCol_MenuBarBg] = ImVec4(0.15f, 0.18f, 0.22f, 1.00f);
      colors[ImGuiCol_ScrollbarBg] = ImVec4(0.02f, 0.02f, 0.02f, 0.39f);
      colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.20f, 0.25f, 0.29f, 1.00f);
      colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.18f, 0.22f, 0.25f, 1.00f);
      colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.09f, 0.21f, 0.31f, 1.00f);
      colors[ImGuiCol_CheckMark] = ImVec4(0.28f, 0.56f, 1.00f, 1.00f);
      colors[ImGuiCol_SliderGrab] = ImVec4(0.28f, 0.56f, 1.00f, 1.00f);
      colors[ImGuiCol_SliderGrabActive] = ImVec4(0.37f, 0.61f, 1.00f, 1.00f);
      colors[ImGuiCol_Button] = ImVec4(0.20f, 0.25f, 0.29f, 1.00f);
      colors[ImGuiCol_ButtonHovered] = ImVec4(0.28f, 0.56f, 1.00f, 1.00f);
      colors[ImGuiCol_ButtonActive] = ImVec4(0.06f, 0.53f, 0.98f, 1.00f);
      colors[ImGuiCol_Header] = ImVec4(0.20f, 0.25f, 0.29f, 0.55f);
      colors[ImGuiCol_HeaderHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
      colors[ImGuiCol_HeaderActive] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
      colors[ImGuiCol_Separator] = ImVec4(0.20f, 0.25f, 0.29f, 1.00f);
      colors[ImGuiCol_SeparatorHovered] = ImVec4(0.10f, 0.40f, 0.75f, 0.78f);
      colors[ImGuiCol_SeparatorActive] = ImVec4(0.10f, 0.40f, 0.75f, 1.00f);
      colors[ImGuiCol_ResizeGrip] = ImVec4(0.26f, 0.59f, 0.98f, 0.25f);
      colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
      colors[ImGuiCol_ResizeGripActive] = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
      colors[ImGuiCol_Tab] = ImVec4(0.11f, 0.15f, 0.17f, 1.00f);
      colors[ImGuiCol_TabHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
      colors[ImGuiCol_TabActive] = ImVec4(0.20f, 0.25f, 0.29f, 1.00f);
      colors[ImGuiCol_TabUnfocused] = ImVec4(0.11f, 0.15f, 0.17f, 1.00f);
      colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.11f, 0.15f, 0.17f, 1.00f);
      colors[ImGuiCol_PlotLines] = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
      colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
      colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
      colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
      colors[ImGuiCol_TextSelectedBg] = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
      colors[ImGuiCol_DragDropTarget] = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
      colors[ImGuiCol_NavHighlight] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
      colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
      colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
      colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);
   }
   else if(index == 4)
   {
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

/*
    [ImGuiCol_Text] = The color for the text that will be used for the whole menu. 
    [ImGuiCol_TextDisabled] = Color for "not active / disabled text". 
    [ImGuiCol_WindowBg] = Background color. 
    [ImGuiCol_PopupBg] = The color used for the background in ImGui :: Combo and ImGui :: MenuBar. 
    [ImGuiCol_Border] = The color that is used to outline your menu. 
    [ImGuiCol_BorderShadow] = Color for the stroke shadow. 
    [ImGuiCol_FrameBg] = Color for ImGui :: InputText and for background ImGui :: Checkbox
    [ImGuiCol_FrameBgHovered] = The color that is used in almost the same way as the one above, except that it changes color when guiding it to ImGui :: Checkbox. 
    [ImGuiCol_FrameBgActive] = Active color. 
    [ImGuiCol_TitleBg] = The color for changing the main place at the very top of the menu (where the name of your "top-of-the-table" is shown. 
    ImGuiCol_TitleBgCollapsed = ImguiCol_TitleBgActive 
    = The color of the active title window, ie if you have a menu with several windows , this color will be used for the window in which you will be at the moment. 
    [ImGuiCol_MenuBarBg] = The color for the bar menu. (Not all sawes saw this, but still)
    [ImGuiCol_ScrollbarBg] = The color for the background of the "strip", through which you can "flip" functions in the software vertically. 
    [ImGuiCol_ScrollbarGrab] = Color for the scoll bar, ie for the "strip", which is used to move the menu vertically. 
    [ImGuiCol_ScrollbarGrabHovered] = Color for the "minimized / unused" scroll bar. 
    [ImGuiCol_ScrollbarGrabActive] = The color for the "active" activity in the window where the scroll bar is located. 
    [ImGuiCol_ComboBg] = Color for the background for ImGui :: Combo. 
    [ImGuiCol_CheckMark] = Color for your ImGui :: Checkbox. 
    [ImGuiCol_SliderGrab] = Color for the slider ImGui :: SliderInt and ImGui :: SliderFloat. 
    [ImGuiCol_SliderGrabActive] = Color of the slider,
    [ImGuiCol_Button] = the color for the button. 
    [ImGuiCol_ButtonHovered] = Color when hovering over the button. 
    [ImGuiCol_ButtonActive] = Button color used. 
    [ImGuiCol_Header] = Color for ImGui :: CollapsingHeader. 
    [ImGuiCol_HeaderHovered] = Color, when hovering over ImGui :: CollapsingHeader. 
    [ImGuiCol_HeaderActive] = Used color ImGui :: CollapsingHeader. 
    [ImGuiCol_Column] = Color for the "separation strip" ImGui :: Column and ImGui :: NextColumn. 
    [ImGuiCol_ColumnHovered] = Color, when hovering on the "strip strip" ImGui :: Column and ImGui :: NextColumn. 
    [ImGuiCol_ColumnActive] = The color used for the "separation strip" ImGui :: Column and ImGui :: NextColumn.
    [ImGuiCol_ResizeGrip] = The color for the "triangle" in the lower right corner, which is used to increase or decrease the size of the menu. 
    [ImGuiCol_ResizeGripHovered] = Color, when hovering to the "triangle" in the lower right corner, which is used to increase or decrease the size of the menu. 
    [ImGuiCol_ResizeGripActive] = The color used for the "triangle" in the lower right corner, which is used to increase or decrease the size of the menu. 
    [ImGuiCol_CloseButton] = The color for the button-closing menu. 
    [ImGuiCol_CloseButtonHovered] = Color, when you hover over the button-close menu. 
    [ImGuiCol_CloseButtonActive] = The color used for the button-closing menu.
    [ImGuiCol_TextSelectedBg] = The color of the selected text, in ImGui :: MenuBar. 
    [ImGuiCol_ModalWindowDarkening] = The color of the "Blackout Window" of your menu. 
    I rarely see these designations, but still decided to put them here. 
    [ImGuiCol_Tab] = The color for tabs in the menu. 
    [ImGuiCol_TabActive] = The active color of tabs, ie when you click on the tab you will have this color.
    [ImGuiCol_TabHovered] = The color that will be displayed when hovering on the table. 
    [ImGuiCol_TabSelected] = The color that is used when you are in one of the tabs. 
    [ImGuiCol_TabText] = Text color that only applies to tabs. 
    [ImGuiCol_TabTextActive] = Active text color for tabs.
*/
}