#pragma once

#include <vector>
// main class for Editor
// only available if built with #define EASE_EDITOR

#include "glad/glad.h"
#include "Renderer/Framebuffer/GLFramebuffer.hpp"
#include "Application/Window/Window.hpp"
#include "Application/Application.hpp"
#include "Renderer/Texture/Texture.hpp"

#include "Scene/SceneTree/Node.hpp"

#include "imgui/imgui.h"



namespace Ease
{

enum class EditorCameraType
{
   NONE = 0,
   TWO_D,
   THREE_D,
};
enum class EditorGameStartResult
{
   NONE = 0,
   START,
   STOP
};

struct EditorData
{
   GLFramebuffer* finalFrameBuffer = nullptr;
   Window* window = nullptr;

   Texture* tex_PlayButton = nullptr;
   Texture* tex_StopButton = nullptr;
   Texture* tex_Directory = nullptr;
   Texture* tex_File = nullptr;
   std::string selectedItemFilename = std::string();

   Node* selectedNode = nullptr;

   bool gameRunning = false;

   Comp::Transform3D* camera2DTransform = nullptr;
   Comp::Transform3D* camera3DTransform = nullptr;
   Comp::Camera* editorCamera2D = nullptr;
   Comp::Camera* editorCamera3D = nullptr;


   EditorCameraType editorCameraType = EditorCameraType::TWO_D;
};

struct EditorReturnData
{
// resets every frame

   EditorGameStartResult gameStartResult = EditorGameStartResult::NONE;

   // for dragging viewport camera
   ImVec2 viewportDragDelta = ImVec2(0.f, 0.f);
   ImVec2 viewportScrollDelta = ImVec2(0.f, 0.f);

// viewport.cpp sets it every frame by copying from editorData
   EditorCameraType editorCameraType = EditorCameraType::TWO_D;
};


class Editor
{
public:
   Editor() {}
   ~Editor();

// initialize editor and imgui
   void init();

   void beginDraw();
   void endDraw();

   EditorReturnData update();

   EditorData editorData;

   static void printLine(const std::string& line);
private:
   void drawMenuBar();
   void drawViewport();
   void drawContentBrowser();
   void drawInspector();
   void drawSceneHierarchy();
   void drawConsole();
   
   // struct that update will return
   // every frame it resets
   EditorReturnData m_EditorReturnData;
};

   
} // namespace Ease
