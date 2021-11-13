#pragma once

#include "Window/Window.hpp"
#include "Renderer/Shader/Shader.hpp"
#include "Renderer/Model/Model.hpp"

#include "Scene/SceneTree/SceneTree.hpp"
#include "Scene/SceneTree/Node.hpp"
#include "Scene/Components.hpp"

#include <memory>

namespace Ease
{

class Application
{
public:
   void Run();

   void setIsRunning();
private:
   Window m_Window;

   std::shared_ptr<Model> m_Sprite2DModel;
   Shader m_Sprite2DShader{};


// sets final framebuffers position relative to window mode (keep width, keep height, stretch etc.) and draws it to screen
   void drawFinalFramebuffer();
   Node* m_FinalFramebufferNode;

   Shader m_PostProcessShader{};

// is game running -> true by default on exports
   bool isRunning = false;

   void startGame();
   void stopGame();

   Comp::Transform3D m_EditorCamera2DTransform;
   Comp::Transform3D m_EditorCamera3DTransform;
   Comp::Camera m_EditorCamera2D;
   Comp::Camera m_EditorCamera3D;
};
   
} // namespace Ease
